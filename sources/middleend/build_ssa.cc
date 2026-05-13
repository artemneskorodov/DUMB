#include <algorithm>
#include <compare>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <iostream>

#include "build_ssa.hh"
#include "ir.hh"
#include "graph.hh"

#include "dot_graph/graph.h"

namespace dumb
{
namespace build_ssa
{

namespace
{

using BBGraph = graph::Graph<ir::BasicBlockID>;

BBGraph
BuildControlFlowGraph( const ir::Function& func)
{
    BBGraph result;

    for ( const ir::BasicBlock& bb : func.BasicBlocks() )
    {
        result.AddNode( bb.id);
    }

    for ( const ir::BasicBlock& bb : func.BasicBlocks() )
    {
        if ( bb.terminator.type == ir::CmpType::INVALID )
        {
            continue;
        }
        result.AddEdge( bb.id, bb.terminator.true_dest);
        if ( bb.terminator.type != ir::CmpType::ALWAYS_TRUE )
        {
            result.AddEdge( bb.id, bb.terminator.false_dest);
        }
    }
    return result;
}

#if 0

///
/// @todo Move this to graph library
///
void
DrawGraph( const std::string& filename,
           const BBGraph&     graph)
{
    dot_graph::Graph dot{ "Unnamed"};

    for ( ir::BasicBlockID id : graph.UsedIds() )
    {
        dot.addNode( "node_" + std::to_string( id));
        for ( ir::BasicBlockID next : graph.GetNexts( id) )
        {
            dot.addEdge( "node_" + std::to_string( id), "node_" + std::to_string( next));
        }
    }
    dot.translateWithDot( filename, "svg");
}

#endif

std::vector<ir::BasicBlockID>
GetVarDefinitionBlocks( const ir::Function& func,
                        nt::SymbolID        var_id)
{
    std::vector<ir::BasicBlockID> def_blocks{};

    for ( const ir::BasicBlock& bb : func.BasicBlocks() )
    {
        for ( const ir::Instruction& instr : bb.instructions )
        {
            if ( instr.defines.type == ir::Operand::VARIABLE &&
                 instr.defines.id   == var_id )
            {
                def_blocks.emplace_back( bb.id);
                break;
            }
        }
    }
    return def_blocks;
}

void
AddPhi( ir::Function&  func,
        const BBGraph& control_flow,
        const BBGraph& dom_frontier)
{
    for ( const ir::SSAKey& var_id : func.Variables() )
    {
        std::vector<ir::BasicBlockID> def_blocks = GetVarDefinitionBlocks( func, var_id.id);
        std::vector<bool> has_phi( control_flow.Size(), false);
        while ( !def_blocks.empty() )
        {
            ir::BasicBlockID block_id = def_blocks.back();
            def_blocks.pop_back();

            for ( ir::BasicBlockID dom_id : dom_frontier.GetNexts( block_id) )
            {
                ir::BasicBlock& dom = func.GetBasicBlock( dom_id);
                if ( !has_phi[dom_id] )
                {
                    dom.phi_nodes.emplace_back( var_id.id);
                    has_phi[dom_id] = true;
                }
                if ( std::find( def_blocks.begin(), def_blocks.end(), dom_id) != def_blocks.end() )
                {
                    def_blocks.emplace_back( dom_id);
                }
            }
        }
    }
}

void
RenameVariables( ir::Function&  function,
                 const BBGraph& dom_tree)
{
    std::unordered_map<nt::SymbolID, std::vector<int>> version_stacks;
    std::unordered_map<nt::SymbolID, int>              counters;

    struct FrameInfo
    {
        ir::BasicBlockID bb_id;
        bool             is_exit;

    };
    std::vector<FrameInfo> workqueue;

    workqueue.push_back( FrameInfo{ 0, false}); // Adding entry to basic block

    while ( !workqueue.empty() )
    {
        FrameInfo frame_info = workqueue.back();
        workqueue.pop_back();

        ir::BasicBlock& basic_block = function.GetBasicBlock( frame_info.bb_id);

        if ( !frame_info.is_exit )
        {
            for ( ir::PhiNode& phi : basic_block.phi_nodes )
            {
                nt::SymbolID id = phi.var.id;
                bool first_time = false;
                if ( counters.find( id) == counters.end() )
                {
                    first_time = true;
                    counters[id] = 0;
                }
                int version = counters[id]++;

                version_stacks[id].push_back( version);
                phi.var.value = version;
                if ( !first_time )
                {
                    function.AddVariable( id, version);
                }
            }

            for ( ir::Instruction& instr : basic_block.instructions )
            {
                for ( ir::Operand& operand : instr.operands )
                {
                    if ( !version_stacks[operand.id].empty() )
                    {
                        operand.value = version_stacks[operand.id].back();
                    }
                }
                if ( instr.defines.type == ir::Operand::VARIABLE )
                {
                    nt::SymbolID id = instr.defines.id;
                    bool first_time = false;
                    if ( counters.find( id) == counters.end() )
                    {
                        first_time = true;
                        counters[id] = 0;
                    }
                    int version = counters[id]++;

                    version_stacks[id].push_back( version);

                    instr.defines.value = version;
                    if ( !first_time )
                    {
                        function.AddVariable( id, version);
                    }
                }
            }

            ir::Operand& left = basic_block.terminator.left;
            if ( left.type == ir::Operand::VARIABLE &&
                 !version_stacks[left.id].empty() )
            {
                left.value = version_stacks[left.id].back();
            }

            ir::Operand& right = basic_block.terminator.right;
            if ( right.type == ir::Operand::VARIABLE &&
                 !version_stacks[right.id].empty() )
            {
                right.value = version_stacks[right.id].back();
            }

            if ( basic_block.terminator.type == ir::CmpType::INVALID )
            {
                // Nothing
            } else
            {
                std::size_t id = basic_block.terminator.true_dest;
                for ( ir::PhiNode& phi : function.GetBasicBlock( id).phi_nodes )
                {
                    if ( !version_stacks[phi.var.id].empty() )
                    {
                        phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::VARIABLE,
                                                                   version_stacks[phi.var.id].back(),
                                                                   phi.var.id};
                    } else
                    {
                        phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::IMMEDIATE, 0};
                    }
                    basic_block.AddPhiAcceptor( id);
                }
                if ( basic_block.terminator.type != ir::CmpType::ALWAYS_TRUE )
                {
                    id = basic_block.terminator.false_dest;
                    for ( ir::PhiNode& phi : function.GetBasicBlock( id).phi_nodes )
                    {
                        if ( !version_stacks[phi.var.id].empty() )
                        {
                            phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::VARIABLE,
                                                                       version_stacks[phi.var.id].back(),
                                                                       phi.var.id};
                        } else
                        {
                            phi.mapping[basic_block.id] = ir::Operand{ ir::Operand::IMMEDIATE, 0};
                        }
                        basic_block.AddPhiAcceptor( id);
                    }
                }
            }

            workqueue.push_back( { basic_block.id, true}); // Planning basic block exit

            for ( ir::BasicBlockID dom : dom_tree.GetNexts( basic_block.id) )
            {
                workqueue.push_back( FrameInfo{ dom, false});
            }
        } else
        {
            for ( ir::PhiNode& phi : basic_block.phi_nodes )
            {
                version_stacks[phi.var.id].pop_back();
            }

            for ( ir::Instruction& instr : basic_block.instructions )
            {
                if ( instr.defines.type == ir::Operand::VARIABLE )
                {
                    version_stacks[instr.defines.id].pop_back();
                }
            }
        }
    }
}

} // ! anonymous namespace

void
BuildSSA( ir::Program& ir,
          int          skip_func)
{
    for ( ir::Function& func : ir.Functions() )
    {
        if ( func.Id() == skip_func )
        {
            continue;
        }
        BBGraph control_flow = BuildControlFlowGraph( func);
        control_flow.BuildDominatorsTable();
        BBGraph dom_tree = graph::BuildDominatorsTree( control_flow);
        BBGraph dom_frontier = graph::BuildDominanceFrontier( control_flow, dom_tree);

        AddPhi( func, control_flow, dom_frontier);
    }

    for ( ir::Function& func : ir.Functions() )
    {
        if ( func.Id() == skip_func )
        {
            continue;
        }
        graph::Graph<ir::BasicBlockID> control_flow = BuildControlFlowGraph( func);
        control_flow.BuildDominatorsTable();
        BBGraph dom_tree = graph::BuildDominatorsTree( control_flow);

        RenameVariables( func, dom_tree);
    }

    return ;
}

} // ! namespace build_ssa
} // ! namespace dumb
