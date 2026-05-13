#include "iv_optimization.hh"
#include "graph.hh"
#include "loops_search.hh"
#include "logger.hh"
#include "inductive_variables.hh"

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
        std::vector<BasicInductionVar> basic_inductions = GetInductiveVariables( func, loops_info);
        for ( const BasicInductionVar& ind_var : basic_inductions )
        {
            LOGGER(LOOP_ANALYSIS) << "Induction var: init = " << ind_var.init.ToStr()
                                  << ", step = " << ind_var.step.ToStr()
                                  << ", in loop(header=" << ind_var.loop->header
                                  << ", latch=" << ind_var.loop->latch << ")";

        }
    }
}

} // ! namespace iv
} // ! namespace dumb
