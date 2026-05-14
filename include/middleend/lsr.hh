#ifndef DUMB_LSR_HH__
#define DUMB_LSR_HH__

#include "ir.hh"
#include "iv_search.hh"

namespace dumb
{
namespace lsr
{

void LoopStrengthReduction( ir::Program&                 program,
                            ir::Function&                func,
                            const iv::DerivedIndVarList& derived_inductions);

} // ! namespace lsr
} // ! namespace dumb

#endif // ! DUMB_LSR_HH__
