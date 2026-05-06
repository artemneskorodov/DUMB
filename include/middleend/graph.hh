#ifndef DUMB_GRAPH_HH__
#define DUMB_GRAPH_HH__

#include <cstddef>
#include <list>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <bitset>
#include <stdexcept>
#include <string>

namespace dumb
{
namespace graph
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

        operator bool()
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
        for ( std::size_t i = 0; i != flags_.size(); ++i )
        {
            result.flags_[i] = flags_[i] & other.flags_[i];
        }
        return result;
    }

    void
    SetAll( bool value)
    {
        PageType page_val = (value ? (~0) : 0);
        for ( PageType& page : flags_ )
        {
            page = page_val;
        }
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

        std::size_t padding_size = flags_.size() * sizeof( PageType) * CHAR_BIT - size_;
        PageType mask = ~((1 << padding_size) - 1);
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

private:
    void
    set( std::size_t bit,
         bool        value)
    {
        check_bit( bit);

        std::size_t page_offset = bit_page( bit);
        std::size_t bit_offset  = bit % (sizeof( PageType) * CHAR_BIT);

        PageType page = flags_[page_offset];
        PageType mask = 1 << bit_offset;

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
        PageType mask = 1 << bit_offset;

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
            return (bit + sizeof( PageType) * CHAR_BIT - 1) / (sizeof( PageType) * CHAR_BIT);
        }
    }

private:
    std::vector<PageType> flags_;
    std::size_t           size_;

};

template<typename NodeIdT>
class Dominators
{
public:
    Dominators( std::size_t                         nodes_number,
                std::function<std::size_t(NodeIdT)> id_to_size_mapper)
     :  flags_             { nodes_number},
        id_to_size_mapper_ { std::move( id_to_size_mapper)}
    {
    }

private:
    class Proxy
    {
    public:
        Proxy( Dominators<NodeIdT>& owner,
               NodeIdT              id)
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
        Dominators<NodeIdT>& owner_;
        NodeIdT              id_;

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

    Dominators
    operator&( const Dominators<NodeIdT>& other) const
    {
        return Dominators{ flags_ & other.flags_, id_to_size_mapper_};
    }

    bool
    operator==( const Dominators<NodeIdT>& other) const
    {
        return flags_ == other.flags_;
    }

    bool
    operator!=( const Dominators<NodeIdT>& other) const
    {
        return !(flags_ == other.flags_);
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

private:
    Dominators( Bitset                              flags,
                std::function<std::size_t(NodeIdT)> id_to_size_mapper)
     :  flags_             { std::move( flags)},
        id_to_size_mapper_ { std::move( id_to_size_mapper)}
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
        bool old_value = flags_[index];
        if ( value && !old_value )
        {
            ++bits_on_;
        } else if ( !value && old_value )
        {
            --bits_on_;
        }

        flags_[index] = value;
    }

private:
    bool
    get( NodeIdT id) const
    {
        return GetByIndex( id_to_size_mapper_( id));
    }

    void
    set( NodeIdT id,
         bool    value)
    {
        SetByIndex( id_to_size_mapper_( id), value);
    }

private:
    Bitset                              flags_;
    std::size_t                         bits_on_;
    std::function<std::size_t(NodeIdT)> id_to_size_mapper_;

};

template<typename NodeIdT>
class DominatorsTable
{
public:
    DominatorsTable( std::size_t                         nodes_number,
                     std::function<std::size_t(NodeIdT)> id_to_size_mapper,
                     std::function<NodeIdT(std::size_t)> size_to_id_mapper)
     :  dominators_( nodes_number, Dominators<NodeIdT>{ nodes_number, id_to_size_mapper}),
        id_to_size_mapper_{ std::move( id_to_size_mapper)},
        size_to_id_mapper_{ std::move( size_to_id_mapper)}
    {
    }

    Dominators<NodeIdT>&
    operator[]( NodeIdT id) &
    {
        return dominators_[ id_to_size_mapper_( id)];
    }

    const Dominators<NodeIdT>&
    operator[]( NodeIdT id) const &
    {
        return dominators_[ id_to_size_mapper_( id)];
    }

    bool
    Dominates( NodeIdT dominator,
               NodeIdT other) const
    {
        return dominates_by_index( id_to_size_mapper_( dominator), id_to_size_mapper_( other));
    }

    bool
    ImmDominates( NodeIdT dominator,
                  NodeIdT other) const
    {
        return imm_dominates_by_index( id_to_size_mapper_( dominator), id_to_size_mapper_( other));
    }

    NodeIdT
    Closest( NodeIdT id) const
    {
        std::size_t node_i = id_to_size_mapper_( id);

        for ( std::size_t other = 0; other != dominators_.size(); ++other )
        {
            if ( !imm_dominates_by_index( node_i, other) )
            {
                continue;
            }

            bool is_closest = true;
            for ( std::size_t between = 0; between != dominators_.size(); ++between )
            {
                if ( !imm_dominates_by_index( between, other) ||
                     !imm_dominates_by_index( node_i, between) )
                {
                    continue ;
                }
                is_closest = false;
                break;
            }
            if ( is_closest )
            {
                return size_to_id_mapper_( other);
            }
        }
        throw std::runtime_error{ "No closest dominator"};
    }

    void
    SetAll( bool value)
    {
        for ( Dominators<NodeIdT>& dom : dominators_ )
        {
            dom.SetAll( value);
        }
    }

private:
    bool
    dominates_by_index( std::size_t dominator,
                        std::size_t other) const
    {
        return dominators_[dominator].GetByIndex( other);
    }

    bool
    imm_dominates_by_index( std::size_t dominator,
                            std::size_t other) const
    {
        return (dominator != other) && dominates_by_index( dominator, other);
    }

private:
    std::vector<Dominators<NodeIdT>>    dominators_;
    std::function<std::size_t(NodeIdT)> id_to_size_mapper_;
    std::function<NodeIdT(std::size_t)> size_to_id_mapper_;

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
        used_ids_.emplace_back( id);

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
        // Removing from used ids
        used_ids_.remove( id);

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

        dominators_table_ = DominatorsTable<NodeIdT>{ nodes_.size(),
                                                      [this](NodeIdT id)->std::size_t
                                                      {
                                                          return id_to_size_mapper( id);
                                                      },
                                                      [this](std::size_t size)->NodeIdT
                                                      {
                                                          return size_to_id_mapper( size);
                                                      }};

        dominators_table_.SetAll( true);
        dominators_table_[entry_].SetAll( false);
        dominators_table_[entry_][entry_] = true;

        bool changed = true;
        while ( changed )
        {
            changed = false;

            for ( NodeIdT node_id : used_ids_ )
            {
                const Node& node = *nodes_hash_[node_id];
                Dominators<NodeIdT> tmp = dominators_table_[node_id];

                for ( NodeIdT pred_id : node.preds )
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

    const std::list<NodeIdT>&
    UsedIds() const &
    {
        return used_ids_;
    }

    std::size_t
    Size() const
    {
        return nodes_.size();
    }

private:
    // TODO rework this mappers
    std::size_t
    id_to_size_mapper( NodeIdT id) const
    {
        const Node *node = nodes_hash_.at( id);

        std::size_t counter = 0;
        for ( const Node& node_in_list : nodes_ )
        {
            if ( node == &node_in_list )
            {
                return counter;
            }

            ++counter;
        }
        return counter;
    }

    // TODO rework this mappers
    NodeIdT
    size_to_id_mapper( std::size_t size) const
    {
        auto it = nodes_.begin();
        for ( std::size_t i = 0; i != size; ++i )
        {
            ++it;
        }
        return it->id;
    }

private:
    std::list<Node>                          nodes_               {};
    std::unordered_map<NodeIdT, Node *>      nodes_hash_          {};
    std::list<NodeIdT>                       used_ids_            {};
    NodeIdT                                  entry_               { 0};
    DominatorsTable<NodeIdT>                 dominators_table_    { 0, {}, {}};
    bool                                     dom_table_actual_    { false};

};

template<typename NodeIdT>
inline Graph<NodeIdT>
BuildDominatorsTree( Graph<NodeIdT> control_flow)
{
    control_flow.BuildDominatorsTable();
    const DominatorsTable<NodeIdT> dom_table = control_flow.GetDominatorsTable();

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
    DominatorsTable<NodeIdT> dom_table = control_flow.GetDominatorsTable();

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
            while ( !dom_table.ImmDominates( node_id, current_id) )
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
