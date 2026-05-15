#ifndef DUMB_FRONTEND_HH__
#define DUMB_FRONTEND_HH__

#include <string>

#include "ir.hh"

namespace dumb
{

ir::Program RunFrontend( const std::string& filename);

void RunASTInterpreter( const std::string& filename);

std::string GeneratePolish( const std::string& filename);

} // ! namespace dumb

#endif // ! DUMB_FRONTEND_HH__
