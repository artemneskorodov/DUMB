#include "middleend.hh"
#include "ir.hh"
#include "ir_dump.hh"
#include "build_ssa.hh"

namespace dumb
{

void
RunMiddleend( ir::Program& program)
{
    build_ssa::BuildSSA( program);
    ir::dump::DumpIR( program, "ssa_ir_dump.svg");
}

} // ! namespace dumb
