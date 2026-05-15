#ifndef DUMB_POLISH_NOTATION_HH__
#define DUMB_POLISH_NOTATION_HH__

#include <string>

#include "ast.hh"

namespace dumb
{
namespace pn
{

std::string GeneratePolishNotation( const ast::Program& ast);

} // ! namespace pn
} // ! namespace dumb

#endif // ! DUMB_POLISH_NOTATION_HH__
