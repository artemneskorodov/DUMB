#ifndef DUMB_AST_INTERPRETER_HH__
#define DUMB_AST_INTERPRETER_HH__

#include "ast.hh"

namespace dumb
{
namespace ast
{
namespace interpreter
{

void Run( const ast::Program& ast);

} // ! namespace interpreter
} // ! namespace ast
} // ! namespace dumb

#endif // ! DUMB_AST_INTERPRETER_HH__
