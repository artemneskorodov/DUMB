#ifndef DUMB_MIDDLEEND_HH__
#define DUMB_MIDDLEEND_HH__

#include "ir.hh"
#include "options.hh"

namespace dumb
{

void RunMiddleend( ir::Program& program, const option::OptionsParser& options);

} // ! namespace dumb

#endif // ! DUMB_MIDDLEEND_HH__
