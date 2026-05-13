#include "iv_optimization.hh"
#include "graph.hh"
#include "loops_search.hh"
#include "logger.hh"
#include "iv_search.hh"

namespace dumb
{
namespace iv
{

namespace
{

graph::Graph<ir::BasicBlockID>
BuildControlFlowGraph( const ir::Function& func)
{
    graph::Graph<ir::BasicBlockID> result;

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

} // ! anonymous namespace

void
InductionVariablesOptimization( ir::Program& program)
{
    for ( ir::Function& func : program.Functions() )
    {
        graph::Graph<ir::BasicBlockID> cfg = BuildControlFlowGraph( func);
        cfg.BuildDominatorsTable();

        // Creating loops tree
        loops::LoopsInfo loops_info{ cfg};
        LOGGER(LOOP_ANALYSIS) << loops_info.ToStr();

        // Creating basic induction variables list
        BasicIndVarList basic_inductions = GetInductiveVariables( func, loops_info);
        for ( const BasicInductionVar& basic_ind : basic_inductions )
        {
            LOGGER(LOOP_ANALYSIS) << "Basic induction: " << basic_ind.ToStr();
        }

        DerivedIndVarList derived_inductions = GetDerivedInductiveVariables( func,
                                                                             loops_info,
                                                                             basic_inductions);
        for ( const DerivedInductionVar& derived_ind : derived_inductions )
        {
            LOGGER(LOOP_ANALYSIS) << "Derived induction: " << derived_ind.ToStr();
        }
    }
}

} // ! namespace iv
} // ! namespace dumb
