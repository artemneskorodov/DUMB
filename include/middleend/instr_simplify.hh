#ifndef DUMB_INSTR_SIMPLIFY_HH__
#define DUMB_INSTR_SIMPLIFY_HH__

#include "ir.hh"

namespace dumb
{
namespace instr_simplify
{

void InstructionsSimplification( ir::Program& program, nt::SymbolID skip_optimizations);

} // ! namespace instr_simplify
} // ! namespace dumb

#endif // ! DUMB_INSTR_SIMPLIFY_HH__
