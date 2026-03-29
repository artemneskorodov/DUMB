#include <vector>
#include <compare>

#include "build_ssa.hh"
#include "ir.hh"

#include "dot_graph/graph.h"

namespace dumb
{
namespace build_ssa
{

namespace
{

class Dominators
{
public:
    Dominators( std::size_t size)
     :  bits_size_{ size},
        flags_( (size + sizeof( uint64_t) - 1) / sizeof( uint64_t), 0)
    {
    }

    class BitProxy
    {
    public:
        BitProxy( Dominators& doms,
                  std::size_t offset,
                  std::uint64_t *pos)
         :  doms_{ doms},
            offset_{ offset},
            pos_{ pos}
        {
        }

        void
        operator=( bool rhs)
        {
            std::size_t qword_offset = offset_ / (sizeof( uint64_t) * 8);
            std::size_t bit_offset   = offset_ % (sizeof( uint64_t) * 8);

            std::uint64_t bit = (rhs ? 1 : 0) << bit_offset;
            std::uint64_t val = doms_.flags_[qword_offset];
            bool old_value = val & (1 << bit_offset);
            if ( rhs && !old_value )
            {
                ++doms_.bits_set_;
            } else if ( !rhs && old_value )
            {
                --doms_.bits_set_;
            }
            val &= ~(1 << bit_offset);
            val |= bit;
            doms_.flags_[qword_offset] = val;

            if ( rhs && offset_ < doms_.head_ )
            {
                doms_.head_ = offset_;
            } else if ( !rhs && offset_ > doms_.head_ )
            {
                bool found = false;
                for ( size_t i = qword_offset; i != doms_.flags_.size(); ++i )
                {
                    std::uint64_t value = doms_.flags_[i];
                    if ( value != 0 )
                    {
                        std::size_t non_zero_bit_offset = 0;
                        while ( (value & 0x1) == 0 )
                        {
                            ++non_zero_bit_offset;
                            value = value >> 1;
                        }
                        doms_.head_ = i * sizeof( uint64_t) + non_zero_bit_offset;
                        found = true;
                        break;
                    }
                }
                if ( !found )
                {
                    doms_.head_ = SIZE_T_MAX;
                }
            }
        }

        operator bool()
        {
            std::uint64_t val = *pos_;
            val &= (1 << offset_);
            return static_cast<bool>( val);
        }

    private:
        Dominators& doms_;
        std::size_t offset_;
        std::uint64_t *pos_;

    };

    BitProxy
    operator[]( std::size_t i)
    {
        std::size_t offset = i % 8;
        std::uint64_t *pos = &flags_[0] + i / 8;
        return BitProxy{ *this, offset, pos};
    };

    static Dominators
    Intersect( /*const*/ Dominators& first, /*const*/ Dominators& second)
    {
        // size_t first_sz = first.flags_.size();
        // size_t second_sz = second.flags_.size();
        Dominators result{ std::max( first.bits_size_, second.bits_size_)};
        // for ( size_t i = 0; i != first_sz && i != second_sz; ++i )
        // {
            // result.flags_[i] = first.flags_[i] & second.flags_[i];
        // }
        for ( std::size_t i = 0; i != std::min( first.bits_size_, second.bits_size_); ++i )
        {
            result[i] = first[i] && second[i];
        }
        return result;
    }

    void
    SetAll( bool value)
    {
        std::uint8_t val;
        if ( value )
        {
            head_ = 0;
            bits_set_ = bits_size_;
            val = 0xff;
        } else
        {
            head_ = SIZE_T_MAX;
            bits_set_ = 0;
            val = 0;
        }
        std::memset( &flags_[0], val, flags_.size() * sizeof( std::uint64_t));
    }

    bool
    operator==( /*const*/ Dominators& other)
    {
        if ( other.bits_size_ != bits_size_ )
        {
            return false;
        }
        for ( size_t i = 0; i != bits_size_; ++i )
        {
            if ( operator[]( i) != other[i] )
            {
                return false;
            }
        }
        return true;
    }

    bool
    operator!=( /*const*/ Dominators& other)
    {
        return !operator==( other);
    }

    std::size_t
    Head() const
    {
        return head_;
    }

    bool
    Empty() const
    {
        return (bits_set_ == 0);
    }

    std::size_t
    Size() const
    {
        return bits_set_;
    }

private:
    std::size_t           bits_size_;
    std::vector<uint64_t> flags_;
    std::size_t           bits_set_ { 0};
    std::size_t           head_     { SIZE_T_MAX};

};

class DominatorsTable
{
public:
    DominatorsTable( std::size_t n)
     :  doms_( n, Dominators( n))
    {
    }

    Dominators&
    operator[]( size_t i) &
    {
        return doms_[i];
    }

    const Dominators&
    operator[]( size_t i) const &
    {
        return doms_[i];
    }

    bool
    Dominates( std::size_t node,
               std::size_t dominator)
    {
        return doms_[node][dominator];
    }

    bool
    ImmDominates( std::size_t node,
                  std::size_t dominator)
    {
        return (node != dominator) && Dominates( node, dominator);
    }

    size_t
    Size() const
    {
        return doms_.size();
    }

    std::size_t
    Closest( std::size_t node)
    {
        for ( std::size_t i = 0; i != doms_.size(); ++i )
        {
            if ( i == node )
            {
                continue;
            }
            if ( !Dominates( node, i) )
            {
                continue;
            }

            bool is_immediate_dominator = true;
            for ( std::size_t k = 0; k != doms_.size(); ++k )
            {
                if ( k == i || k == node )
                {
                    continue;
                }
                if ( Dominates( k, i) && Dominates( node, k) )
                {
                    is_immediate_dominator = false;
                    break;
                }
            }
            if ( is_immediate_dominator )
            {
                return i;
            }
        }
        throw std::runtime_error{ "No immediate dominator"};
    }

private:
    std::vector<Dominators> doms_;

};

void
print_doms( DominatorsTable& doms)
{
    for ( size_t i = 0; i != doms.Size(); ++i )
    {
        std::cout << i << ": ";
        for ( size_t j = 0; j != doms.Size(); ++j )
        {
            if ( doms[i][j] )
            {
                std::cout << j << " ";
            }
        }
        std::cout << "\n";
    }
}

class Graph
{
public:
    struct Node
    {
        std::vector<std::size_t> nexts;
        std::vector<std::size_t> preds;
    };

public:
    Graph( std::size_t n)
     :  nodes_( n)
    {
    }

    void
    Resize( std::size_t n)
    {
        for ( const auto& node : nodes_ )
        {
            for ( std::size_t next : node.nexts )
            {
                if ( next >= n )
                {
                    throw std::runtime_error{ "Unexpected to resize before removing "};
                }
            }
        }
        nodes_.resize( n);
    }

    std::size_t
    Size() const
    {
        return nodes_.size();
    }

    void
    AddEdge( std::size_t from,
             std::size_t to)
    {
        nodes_[from].nexts.emplace_back( to);
        nodes_[to].preds.emplace_back( from);
    }

    DominatorsTable
    GetDominators() const
    {
        DominatorsTable dominators{ nodes_.size()};
        dominators[0][0] = true;
        for ( std::size_t i = 1; i != nodes_.size(); ++i )
        {
            dominators[i].SetAll( true);
        }

        bool changed = true;
        while ( changed )
        {
            changed = false;
            for ( size_t node_id = 0; node_id != nodes_.size(); ++node_id )
            {
                const Node& node = nodes_[node_id];
                Dominators tmp = dominators[node_id];
                for ( size_t pred : node.preds )
                {
                    tmp = Dominators::Intersect( tmp, dominators[pred]);
                }
                tmp[node_id] = true;
                bool cmp_result = (tmp != dominators[node_id]);
                changed = changed || cmp_result;
                if ( cmp_result )
                {
                    dominators[node_id] = std::move( tmp);
                }
            }
        }
        return dominators;
    }

    const std::vector<Node>&
    Nodes() const &
    {
        return nodes_;
    }

private:
    std::vector<Node> nodes_{};

};

Graph
BuildControlFlowGraph( const ir::Function& func)
{
    Graph graph{ func.BasicBlocks().size()};

    for ( const ir::BasicBlock& bb : func.BasicBlocks() )
    {
        if ( bb.terminator.type == ir::CmpType::INVALID )
        {
            continue;
        }
        graph.AddEdge( bb.id, bb.terminator.true_dest);
        if ( bb.terminator.type != ir::CmpType::ALWAYS_TRUE )
        {
            graph.AddEdge( bb.id, bb.terminator.false_dest);
        }
    }
    return graph;
}

Graph
BuildDominatorsTree( const Graph& control_flow)
{
    Graph tree{ control_flow.Size()};
    DominatorsTable dominators_table = control_flow.GetDominators();

    print_doms( dominators_table);

    for ( std::size_t node_id = 0; node_id != control_flow.Size(); ++node_id )
    {
        Dominators doms = dominators_table[node_id];
        doms[node_id] = false;
        if ( doms.Empty() )
        {
            continue;
        } else if ( doms.Size() == 1 )
        {
            tree.AddEdge( doms.Head(), node_id);
            continue;
        }
        tree.AddEdge( dominators_table.Closest( node_id), node_id);
    }
    return tree;
}

Graph
BuildDominanceFrontier( const Graph& control_flow,
                        const Graph& dominators_tree)
{
    if ( control_flow.Size() != dominators_tree.Size() )
    {
        throw std::runtime_error{ "Dominators tree number of nodes "
                                  "does not equal to control flow "
                                  "number of nodes"};
    }
    Graph dominance_frontier{ control_flow.Size()};
    DominatorsTable dominators = control_flow.GetDominators();

    for ( std::size_t node_id = 0; node_id != control_flow.Size(); ++node_id )
    {
        for ( std::size_t pred_id = 0; pred_id != control_flow.Nodes()[node_id].preds.size(); ++pred_id)
        {
            std::size_t current = control_flow.Nodes()[node_id].preds[pred_id];
            while ( !dominators.ImmDominates( node_id, current) )
            {
                dominance_frontier.AddEdge( current, node_id);
                current = dominators_tree.Nodes()[current].preds[0];
            }
        }
    }
    return dominance_frontier;
}

void
DrawGraph( const std::string& filename, const Graph& graph)
{
    dot_graph::Graph dot{ "Unnamed"};
    for ( size_t i = 0; i != graph.Nodes().size(); ++i )
    {
        dot.addNode( "node_" + std::to_string( i));
        for ( size_t j : graph.Nodes()[i].nexts )
        {
            dot.addEdge( "node_" + std::to_string( i), "node_" + std::to_string( j));
        }
    }
    dot.translateWithDot( filename, "svg");
}

std::vector<std::size_t>
GetVarDefinitionBlocks( const ir::Function& func,
                        nt::SymbolID var_id)
{
    std::vector<std::size_t> def_blocks{};
    for ( std::size_t bb = 0; bb != func.BasicBlocks().size(); ++bb )
    {
        for ( const ir::Instruction& instr : func.BasicBlocks()[bb].instructions )
        {
            if ( instr.defines.type == ir::Operand::VARIABLE &&
                 instr.defines.id   == var_id )
            {
                def_blocks.emplace_back( bb);
                break;
            }
        }
    }
    return def_blocks;
}

void
AddPhi( ir::Program& ir)
{
    for ( ir::Function& func : ir.Functions() )
    {
        Graph control_flow = BuildControlFlowGraph( func);
        Graph dominators_tree = BuildDominatorsTree( control_flow);
        Graph dominance_frontier = BuildDominanceFrontier( control_flow, dominators_tree);

        DrawGraph( "graph_cf.svg", control_flow);
        DrawGraph( "graph_dt.svg", dominators_tree);
        DrawGraph( "graph_df.svg", dominance_frontier);

        for ( int var_id : func.Variables() )
        {
            std::vector<std::size_t> def_blocks = GetVarDefinitionBlocks( func, var_id);

            std::vector<bool> has_phi( control_flow.Size(), false);
            while ( !def_blocks.empty() )
            {
                std::size_t block = def_blocks.back();
                def_blocks.pop_back();

                for ( std::size_t dom : dominance_frontier.Nodes()[block].nexts )
                {
                    ir::BasicBlock& basic_block = func.BasicBlocks()[dom];
                    if ( !has_phi[dom] )
                    {
                        basic_block.phi_nodes.emplace_back( var_id);
                        has_phi[dom] = true;
                    }
                    if ( std::find( def_blocks.begin(), def_blocks.end(), dom) != def_blocks.end() )
                    {
                        def_blocks.emplace_back( dom);
                    }
                }
            }
        }
    }
}

void
RenameVariables( ir::Function& function,
                 ir::BasicBlock& basic_block,
                 Graph& control_flow,
                 Graph& dominators_tree,
                 Graph& dominance_frontier,
                 std::unordered_map<nt::SymbolID, int>& versions_stacks) // TODO FIXME
{
    std::unordered_map<int, int> old_versions_stacks = versions_stacks;
    for ( ir::PhiNode& phi : basic_block.phi_nodes )
    {
        nt::SymbolID id = phi.var_id;
        std::cout << "Id = " << id << std::endl;
        if ( versions_stacks.find( id) == versions_stacks.end() )
        {
            versions_stacks[id] = 0;
            phi.version = 0;
        } else
        {
            ++versions_stacks[id];
            phi.version = versions_stacks[id];
        }
    }

    for ( ir::Instruction& instr : basic_block.instructions )
    {
        for ( ir::Operand& operand : instr.operands )
        {
            if ( operand.type == ir::Operand::VARIABLE &&
                 versions_stacks.find( operand.id) != versions_stacks.end() )
            {
                operand.value = versions_stacks[operand.id];
            }
        }
        if ( instr.defines.type == ir::Operand::VARIABLE &&
             versions_stacks.find( instr.defines.id) != versions_stacks.end() )
        {
            ++versions_stacks[instr.defines.id];
            instr.defines.value = versions_stacks[instr.defines.id];
        }
    }

    if ( basic_block.terminator.type == ir::CmpType::INVALID )
    {
        return ;
    } else
    {
        std::size_t id = basic_block.terminator.true_dest;
        for ( ir::PhiNode& phi : function.BasicBlocks()[id].phi_nodes )
        {
            phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::VARIABLE, versions_stacks[phi.var_id], phi.var_id};
        }
        if ( basic_block.terminator.type != ir::CmpType::ALWAYS_TRUE )
        {
            id = basic_block.terminator.false_dest;
            for ( ir::PhiNode& phi : function.BasicBlocks()[id].phi_nodes )
            {
                phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::VARIABLE, versions_stacks[phi.var_id], phi.var_id};
            }
        }
    }

    for ( std::size_t dom : dominators_tree.Nodes()[basic_block.id].nexts )
    {
        std::cout << "Dom = " << dom << std::endl;
        RenameVariables( function, function.BasicBlocks()[dom], control_flow, dominators_tree, dominance_frontier, versions_stacks);
    }

    versions_stacks = old_versions_stacks;
}

} // ! anonymous namespace

void
BuildSSA( ir::Program& ir)
{
    AddPhi( ir);

    for ( ir::Function& func : ir.Functions() )
    {
        Graph control_flow = BuildControlFlowGraph( func);
        Graph dominators_tree = BuildDominatorsTree( control_flow);
        Graph dominance_frontier = BuildDominanceFrontier( control_flow, dominators_tree);
        std::unordered_map<nt::SymbolID, int> variables_stack;
        RenameVariables( func, func.BasicBlocks()[0], control_flow, dominators_tree, dominance_frontier, variables_stack);
    }

    return ;
    Graph graph{ 8};
    graph.AddEdge( 0, 1);
    graph.AddEdge( 1, 2);
    graph.AddEdge( 2, 3);
    graph.AddEdge( 1, 4);
    graph.AddEdge( 4, 5);
    graph.AddEdge( 4, 6);
    graph.AddEdge( 6, 1);
    graph.AddEdge( 5, 7);
    graph.AddEdge( 6, 7);
    graph.AddEdge( 7, 3);

    DrawGraph( "test_graph.svg", graph);
    Graph dominators_tree = BuildDominatorsTree( graph);
    DrawGraph( "domtree.svg", dominators_tree);
    Graph dominance_frontier = BuildDominanceFrontier( graph, dominators_tree);
    DrawGraph( "domfront.svg", dominance_frontier);
}

} // ! namespace build_ssa
} // ! namespace dumb
