#ifndef DUMB_SCCP_HH__
#define DUMB_SCCP_HH__

#include "ir.hh"

namespace dumb
{
namespace sccp
{

void SparseConditionalConstantPropagation( ir::Program& ir, int skip_func);

} // ! namespace sccp
} // ! namespace dumb

#endif // ! DUMB_SCCP_HH__
