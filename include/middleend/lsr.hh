#ifndef DUMB_LSR_HH__
#define DUMB_LSR_HH__

#include "ir.hh"
#include "iv_search.hh"

namespace dumb
{
namespace lsr
{

void LoopStrengthReduction( ir::Program& program, nt::SymbolID skip_optimizations);

} // ! namespace lsr
} // ! namespace dumb

#endif // ! DUMB_LSR_HH__
