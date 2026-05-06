#ifndef DUMB_GRAPH_HH__
#define DUMB_GRAPH_HH__

#include <cstddef>
#include <list>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <climits>
#include <vector>
#include <bit>

namespace dumb
{
namespace graph
{

namespace detail
{

class Bitset
{
private:
    using PageType = std::uint64_t;

public:
    Bitset( std::size_t bits_number)
     :  flags_ ( bit_page( bits_number, true)),
        size_  { bits_number}
    {
    }

private:
    ///
    /// @brief Proxy class which allows writing with operator []
    ///
    class BitProxy
    {
    public:
        BitProxy( Bitset&     owner,
                  std::size_t bit)
         :  owner_{ owner},
            bit_{ bit}
        {
        }

        void
        operator=( bool value)
        {
            owner_.set( bit_, value);
        }

        operator bool() const
        {
            return owner_.get( bit_);
        }

    private:
        Bitset&     owner_;
        std::size_t bit_;

    };

public:
    bool
    operator[]( std::size_t bit) const
    {
        return get( bit);
    }

    BitProxy
    operator[]( std::size_t bit)
    {
        return BitProxy{ *this, bit};
    }

    Bitset
    operator&( const Bitset& other) const
    {
        if ( size_ != other.size_ )
        {
            throw std::runtime_error{ "Unexpected to intersect bitsets with different sizes"};
        }

        Bitset result{ size_};
        std::size_t last = flags_.size() - 1;
        for ( std::size_t i = 0; i != last; ++i )
        {
            PageType page = flags_[i] & other.flags_[i];
            result.bits_on_ += std::popcount( page);
            result.flags_[i] = page;
        }

        PageType last_page = flags_[last] & other.flags_[last];
        PageType mask = last_page_mask();
        last_page &= mask;
        result.bits_on_ += std::popcount( last_page);
        result.flags_[last] = last_page;

        return result;
    }

    void
    SetAll( bool value)
    {
        PageType page_val = (value ? (~0ull) : 0ull);

        std::size_t last = flags_.size() - 1;
        for ( std::size_t i = 0; i != last; ++i )
        {
            flags_[i] = page_val;
        }
        flags_[last] = page_val & last_page_mask();

        bits_on_ = (value ? size_ : 0);
    }

    bool
    operator==( const Bitset& other) const
    {
        if ( size_ != other.size_ )
        {
            throw std::runtime_error{ "Unexpected to compare bitsets with different size"};
        }

        std::size_t last = flags_.size() - 1;

        for ( std::size_t i = 0; i != last; ++i )
        {
            if ( flags_[i] != other.flags_[i] )
            {
                return false;
            }
        }

        PageType mask = last_page_mask();

        if ( (flags_[last] & mask) != (other.flags_[last] & mask))
        {
            return false;
        }
        return true;
    }

    bool
    operator!=( const Bitset& other) const
    {
        return !(*this == other);
    }

    std::size_t
    Size() const
    {
        return bits_on_;
    }

    bool
    Empty() const
    {
        return (Size() == 0);
    }

    std::string
    ToStr() const
    {
        std::string result{};
        for ( std::size_t bit = 0; bit != size_; ++bit )
        {
            result += ((*this)[bit] ? "1" : "-");
        }
        return result;
    }

private:
    void
    set( std::size_t bit,
         bool        value)
    {
        check_bit( bit);

        std::size_t page_offset = bit_page( bit);
        std::size_t bit_offset  = bit % (sizeof( PageType) * CHAR_BIT);

        PageType page = flags_[page_offset];
        PageType mask = 1ull << bit_offset;

        bool old_value = page & mask;
        if ( value && !old_value )
        {
            ++bits_on_;
        } else if ( !value && old_value )
        {
            --bits_on_;
        }

        if ( value )
        {
            page |= mask;
        } else
        {
            page &= ~mask;
        }

        flags_[page_offset] = page;
    }

    bool
    get( std::size_t bit) const
    {
        check_bit( bit);

        std::size_t page_offset = bit_page( bit);
        std::size_t bit_offset  = bit % (sizeof( PageType) * CHAR_BIT);

        PageType page = flags_[page_offset];
        PageType mask = 1ull << bit_offset;

        return page & mask;
    }

    void
    check_bit( std::size_t bit) const
    {
        if ( bit >= size_ )
        {
            throw std::out_of_range{ "bit = " + std::to_string( bit) +
                                     ", size = " + std::to_string( size_)};
        }
    }

    PageType
    last_page_mask() const
    {
        std::size_t padding = flags_.size() * sizeof( PageType) * CHAR_BIT - size_;

        PageType mask;
        if ( padding == sizeof( PageType) * CHAR_BIT )
        {
            mask = ~(0ull);
        } else
        {
            mask = ~((1ull << padding) - 1ull);
        }
        return mask;
    }

private:
    static std::size_t
    bit_page( std::size_t bit,
              bool        align_up = false)
    {
        if ( !align_up )
        {
            return bit / (sizeof( PageType) * CHAR_BIT);
        } else
        {
            return (bit + sizeof( PageType) * CHAR_BIT - 1ull) / (sizeof( PageType) * CHAR_BIT);
        }
    }

private:
    std::vector<PageType> flags_;
    std::size_t           size_;
    std::size_t           bits_on_{ 0};

};

template<typename NodeIdT>
class IdAndSizeMapper
{
public:
    std::size_t
    IdToSize( NodeIdT id) const
    {
        return id_to_size_map_.at( id);
    }

    NodeIdT
    SizeToId( std::size_t size) const
    {
        return size_to_id_map_[size];
    }

    void
    Add( NodeIdT id)
    {
        id_to_size_map_[id] = size_to_id_map_.size();
        size_to_id_map_.emplace_back( id);
    }

    ///
    /// @brief Removes ID from mapping
    /// @warning This function is very slow
    /// @param id Id to remove
    ///
    void
    Remove( NodeIdT id)
    {
        std::vector<NodeIdT> new_size_to_id( size_to_id_map_.size() - 1);
        for ( NodeIdT node_id : size_to_id_map_ )
        {
            if ( id != node_id )
            {
                new_size_to_id.emplace_back( node_id);
            }
        }
        size_to_id_map_ = std::move( new_size_to_id);
    }

    const std::vector<NodeIdT>&
    UsedNodeIds() const &
    {
        return size_to_id_map_;
    }

private:
    std::unordered_map<NodeIdT, std::size_t> id_to_size_map_{};
    std::vector<NodeIdT>                     size_to_id_map_{};

};

} // ! namespace detail

template<typename NodeIdT>
class Dominators
{
private:
    using this_type_t = Dominators<NodeIdT>;
    using mapper_t = detail::IdAndSizeMapper<NodeIdT>;

public:
    Dominators( std::size_t     nodes_number,
                const mapper_t *mapper)
     :  flags_  { nodes_number},
        mapper_ { mapper}
    {
    }

private:
    class Proxy
    {
    public:
        Proxy( this_type_t& owner,
               NodeIdT      id)
         :  owner_ { owner},
            id_    { id}
        {
        }

        void
        operator=( bool value)
        {
            owner_.set( id_, value);
        }

        operator bool()
        {
            return owner_.get( id_);
        }

    private:
        this_type_t& owner_;
        NodeIdT      id_;

    };

public:
    bool
    operator[]( NodeIdT id) const
    {
        return get( id);
    }

    Proxy
    operator[]( NodeIdT id)
    {
        return Proxy{ *this, id};
    }

    void
    SetAll( bool value)
    {
        flags_.SetAll( value);
    }

    this_type_t
    operator&( const this_type_t& other) const
    {
        check_mappers_same( mapper_, other.mapper_);

        return this_type_t{ flags_ & other.flags_, mapper_};
    }

    bool
    operator==( const this_type_t& other) const
    {
        check_mappers_same( mapper_, other.mapper_);

        return flags_ == other.flags_;
    }

    bool
    operator!=( const this_type_t& other) const
    {
        return !(flags_ == other.flags_);
    }

    std::size_t
    Size() const
    {
        return flags_.Size();
    }

    bool
    Empty() const
    {
        return flags_.Empty();
    }

    std::string
    ToStr() const
    {
        return flags_.ToStr();
    }

private:
    Dominators( detail::Bitset  flags,
                const mapper_t *mapper)
     :  flags_  { std::move( flags)},
        mapper_ { mapper}
    {
    }

public:
    bool
    GetByIndex( std::size_t index) const
    {
        return flags_[index];
    }

    void
    SetByIndex( std::size_t index,
                bool        value)
    {
        flags_[index] = value;
    }

private:
    bool
    get( NodeIdT id) const
    {
        return GetByIndex( mapper_->IdToSize( id));
    }

    void
    set( NodeIdT id,
         bool    value)
    {
        SetByIndex( mapper_->IdToSize( id), value);
    }

    void
    check_mappers_same( const mapper_t *first,
                        const mapper_t *second) const
    {
        // Comparing pointers to ensure this mappers are from same graph
        if ( first != second )
        {
            throw std::runtime_error{ "Different mappers"};
        }
    }

private:
    detail::Bitset  flags_;
    const mapper_t *mapper_;

};

template<typename NodeIdT>
class DominatorsTable
{
private:
    using dominators_type_t = Dominators<NodeIdT>;
    using mapper_t = detail::IdAndSizeMapper<NodeIdT>;

public:
    DominatorsTable( std::size_t     nodes_number,
                     const mapper_t *mapper)
     :  dominators_ ( nodes_number, dominators_type_t{ nodes_number, mapper}),
        mapper_     { mapper}
    {
    }

    dominators_type_t&
    operator[]( NodeIdT id) &
    {
        return dominators_[id];
    }

    const dominators_type_t&
    operator[]( NodeIdT id) const &
    {
        return dominators_[id];
    }

    bool
    Dominates( NodeIdT dominator,
               NodeIdT node) const
    {
        return dominates_by_index( mapper_->IdToSize( dominator), mapper_->IdToSize( node));
    }

    bool
    ImmDominates( NodeIdT dominator,
                  NodeIdT node) const
    {
        return imm_dominates_by_index( mapper_->IdToSize( dominator), mapper_->IdToSize( node));
    }

    NodeIdT
    Closest( NodeIdT id) const
    {
        std::size_t node_i = mapper_->IdToSize( id);

        for ( std::size_t other = 0; other != dominators_.size(); ++other )
        {
            if ( !imm_dominates_by_index( other, node_i) )
            {
                continue;
            }

            bool has_between = true;
            for ( std::size_t between = 0; between != dominators_.size(); ++between )
            {
                if ( !imm_dominates_by_index( other, between) ||
                     !imm_dominates_by_index( between, node_i) )
                {
                    continue ;
                }
                has_between = false;
                break;
            }
            if ( has_between )
            {
                return mapper_->SizeToId( other);
            }
        }
        throw std::runtime_error{ "No closest dominator"};
    }

    void
    SetAll( bool value)
    {
        for ( dominators_type_t& dom : dominators_ )
        {
            dom.SetAll( value);
        }
    }

    std::string
    ToStr() const
    {
        std::string result{};
        std::size_t counter = 0;
        for ( const dominators_type_t& doms : dominators_ )
        {
            result += std::to_string( mapper_->SizeToId( counter)) + " " + doms.ToStr() + "\n";
            ++counter;
        }
        return result;
    }

private:
    bool
    dominates_by_index( std::size_t dominator,
                        std::size_t node) const
    {
        return dominators_[node].GetByIndex(dominator);
    }

    bool
    imm_dominates_by_index( std::size_t dominator,
                            std::size_t node) const
    {
        return (dominator != node) && dominates_by_index( dominator, node);
    }

private:
    std::vector<dominators_type_t> dominators_;
    const mapper_t                *mapper_;

};

template<typename NodeIdT>
class Graph
{
public:
    struct Node
    {
        Node( NodeIdT id)
         :  id{ id}
        {
        }

        std::list<NodeIdT> nexts;
        std::list<NodeIdT> preds;
        NodeIdT              id;

    };

public:
    void
    AddNode( NodeIdT id)
    {
        nodes_.emplace_back( id);
        nodes_hash_[id] = &nodes_.back();
        mapper_.Add( id);

        dom_table_actual_ = false;
    }

    void
    AddEdge( NodeIdT from, NodeIdT to)
    {
        nodes_hash_[from]->nexts.emplace_back( to);
        nodes_hash_[to]->preds.emplace_back( from);

        dom_table_actual_ = false;
    }

    void
    RemoveNode( NodeIdT id)
    {
        // Removing connections
        for ( NodeIdT pred_id : GetPreds( id) )
        {
            Node& node = *nodes_hash_[pred_id];
            node.nexts.remove( id);
        }
        for ( NodeIdT next_id : GetNexts( id) )
        {
            Node& node = *nodes_hash_[next_id];
            node.preds.remove( id);
        }

        // Removing from nodes list
        nodes_hash_.erase( id);
        // Removing from id map
        nodes_.remove_if( [id](const Node& node)->bool { return node.id == id; });
        // Removing from NodeIdT <-> std::size_t mapper
        mapper_.Remove( id);

        dom_table_actual_ = false;
    }

    void
    SetEntry( NodeIdT entry)
    {
        entry_ = entry;

        dom_table_actual_ = false;
    }

    const std::list<NodeIdT>&
    GetNexts( NodeIdT id) const &
    {
        return nodes_hash_.at( id)->nexts;
    }

    const std::list<NodeIdT>&
    GetPreds( NodeIdT id) const &
    {
        return nodes_hash_.at( id)->preds;
    }

    void
    BuildDominatorsTable()
    {
        if ( dom_table_actual_ )
        {
            return ;
        }

        dominators_table_ = DominatorsTable<NodeIdT>{ nodes_.size(), &mapper_};

        dominators_table_.SetAll( true);
        dominators_table_[entry_].SetAll( false);
        dominators_table_[entry_][entry_] = true;

        bool changed = true;
        while ( changed )
        {
            changed = false;

            for ( NodeIdT node_id : UsedIds() )
            {
                Dominators<NodeIdT> tmp = dominators_table_[node_id];

                for ( NodeIdT pred_id : GetPreds( node_id) )
                {
                    tmp = tmp & dominators_table_[pred_id];
                }

                tmp[node_id] = true;

                bool cmp_result = (tmp != dominators_table_[node_id]);
                changed = changed || cmp_result;
                if ( cmp_result )
                {
                    dominators_table_[node_id] = std::move( tmp);
                }
            }
        }
        dom_table_actual_ = true;
    }

    const DominatorsTable<NodeIdT>&
    GetDominatorsTable() const &
    {
        if ( !dom_table_actual_ )
        {
            throw std::runtime_error{ "Call BuildDominatorsTable first"};
        }
        return dominators_table_;
    }

    const std::vector<NodeIdT>&
    UsedIds() const &
    {
        return mapper_.UsedNodeIds();
    }

    std::size_t
    Size() const
    {
        return nodes_.size();
    }

private:
    std::list<Node>                          nodes_               {};
    std::unordered_map<NodeIdT, Node *>      nodes_hash_          {};
    NodeIdT                                  entry_               { 0};
    detail::IdAndSizeMapper<NodeIdT>         mapper_              {};
    DominatorsTable<NodeIdT>                 dominators_table_    { 0, &mapper_};
    bool                                     dom_table_actual_    { false};

};

template<typename NodeIdT>
inline Graph<NodeIdT>
BuildDominatorsTree( const Graph<NodeIdT>& control_flow)
{
    const DominatorsTable<NodeIdT>& dom_table = control_flow.GetDominatorsTable();

    // Copying nodes to dominators tree
    Graph<NodeIdT> tree{};
    for ( NodeIdT node_id : control_flow.UsedIds() )
    {
        tree.AddNode( node_id);
    }

    for ( NodeIdT node_id : control_flow.UsedIds() )
    {
        Dominators<NodeIdT> doms = dom_table[node_id];
        doms[node_id] = false;
        if ( doms.Empty() )
        {
            continue;
        } else
        {
            tree.AddEdge( dom_table.Closest( node_id), node_id);
        }
    }
    return tree;
}

template<typename NodeIdT>
inline Graph<NodeIdT>
BuildDominanceFrontier( const Graph<NodeIdT>& control_flow,
                        const Graph<NodeIdT>& dom_tree)
{
    const DominatorsTable<NodeIdT>& dom_table = control_flow.GetDominatorsTable();

    if ( control_flow.Size() != dom_tree.Size() )
    {
        throw std::runtime_error{ "Expected to call BuildDominanceFrontier"
                                  " only for corresponding control flow and dominators tree"};
    }

    // Copying nodes to dominance frontier graph
    Graph<NodeIdT> dominance_frontier{};
    for ( NodeIdT node_id : control_flow.UsedIds() )
    {
        dominance_frontier.AddNode( node_id);
    }

    for ( NodeIdT node_id : control_flow.UsedIds() )
    {
        for ( NodeIdT pred_id : control_flow.GetPreds( node_id) )
        {
            NodeIdT current_id = pred_id;
            while ( !dom_table.ImmDominates( current_id, node_id) )
            {
                dominance_frontier.AddEdge( current_id, node_id);
                current_id = dom_tree.GetPreds( current_id).front();
            }
        }
    }
    return dominance_frontier;
}

} // ! namespace graph
} // ! namespace dumb

#endif // ! DUMB_GRAPH_HH__
