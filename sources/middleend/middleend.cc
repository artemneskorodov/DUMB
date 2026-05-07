#include "middleend.hh"
#include "ir.hh"
#include "ir_dump.hh"
#include "build_ssa.hh"
#include "options.hh"
#include "sccp.hh"
#include "dce.hh"

namespace dumb
{

void
RunMiddleend( ir::Program&            program,
              const MiddleendOptions& options)
{
    // Skipping main optimizations if building benchmark
    int skip_optimizations = -1;
    if ( options.benchmark_build )
    {
        const nt::Symbol *main_sym = program.Nametable().FindSymbol( "main");
        if ( main_sym == nullptr )
        {
            throw std::runtime_error{ "Program is expected to have 'main' function"};
        }
        skip_optimizations = main_sym->GetID();
    }

    build_ssa::BuildSSA( program, skip_optimizations);
    ir::dump::DumpIR( program, "after_build_ssa");

    if ( options.enable_sccp )
    {
        sccp::SparseConditionalConstantPropagation( program, skip_optimizations);
        ir::dump::DumpIR( program, "after_sccp");
    }

    if ( options.enable_dce )
    {
        dce::DeadCodeElimination( program, skip_optimizations);
        ir::dump::DumpIR( program, "after_dce");
    }
}

} // ! namespace dumb
