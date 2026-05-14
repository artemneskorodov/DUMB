#ifndef DUMB_LICM_HH__
#define DUMB_LICM_HH__

#include "ir.hh"
#include "loops_search.hh"

namespace dumb
{
namespace licm
{

void LoopInvariantCodeMotion( ir::Function& func, loops::LoopsInfo& loops);

} // ! namespace licm
} // ! namespace dumb

#endif // ! DUMB_LICM_HH__
