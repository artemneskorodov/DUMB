#ifndef DUMB_IV_OPTIMIZATION_HH__
#define DUMB_IV_OPTIMIZATION_HH__

#include "ir.hh"
#include "middleend.hh"

namespace dumb
{
namespace iv
{

void InductionVariablesOptimization( ir::Program& program, const MiddleendOptions& options);

} // ! namespace iv
} // ! namespace dumb

#endif // ! DUMB_IV_OPTIMIZATION_HH__
