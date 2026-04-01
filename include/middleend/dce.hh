#ifndef DUMB_DCE_HH__
#define DUMB_DCE_HH__

#include "ir.hh"

namespace dumb
{
namespace dce
{

void DeadCodeElimination( ir::Program& ir);

} // ! namespace dce
} // ! namespace dumb

#endif // ! DUMB_DCE_HH__
