#ifndef DUMB_BUILD_SSA_HH__
#define DUMB_BUILD_SSA_HH__

#include "ir.hh"

namespace dumb
{
namespace build_ssa
{

void BuildSSA( ir::Program& ir, int skip_func);

} // ! namespace build_ssa
} // ! namespace dumb

#endif // ! DUMB_BUILD_SSA_HH__
