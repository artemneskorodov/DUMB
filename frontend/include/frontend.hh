#ifndef DUMB_FRONTEND_HH__
#define DUMB_FRONTEND_HH__

#include "ir.hh"

namespace dumb
{

hir::Program RunFrontend( const std::string& filename);

} // ! namespace dumb

#endif // ! DUMB_FRONTEND_HH__
