#ifndef DUMB_SYNTAX_PARSER_HH__
#define DUMB_SYNTAX_PARSER_HH__

#include <vector>

#include "ast.hh"
#include "lexer.hh"

namespace dumb
{
namespace syntax
{

ast::Program ParseSyntax( const std::vector<lexer::Token>& tokens, const std::string& filename);

} // ! namespace syntax
} // ! namespace dumb

#endif // ! DUMB_SYNTAX_PARSER_HH__
