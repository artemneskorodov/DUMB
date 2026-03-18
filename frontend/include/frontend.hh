#ifndef DUMB_FRONTEND_HH__
#define DUMB_FRONTEND_HH__

#include "ast.hh"

namespace dumb
{

ast::Program RunFrontend( const std::string& filename);

} // ! namespace dumb

#endif // ! DUMB_FRONTEND_HH__
