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
RunMiddleend( ir::Program&           program,
              const option::OptionsParser& options)
{
    build_ssa::BuildSSA( program);
    ir::dump::DumpIR( program, "after_build_ssa");

    bool need_sccp = options.GetOption<bool>( "--enable-sccp");
    if ( need_sccp )
    {
        sccp::SparseConditionalConstantPropagation( program);
        ir::dump::DumpIR( program, "after_sccp");
    }

    bool need_dce = options.GetOption<bool>( "--enable-dce");
    if ( need_dce )
    {
        dce::DeadCodeElimination( program);
        ir::dump::DumpIR( program, "after_dce");
    }
}

} // ! namespace dumb
