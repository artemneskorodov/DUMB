#ifndef DUMB_AST_DUMP_HH__
#define DUMB_AST_DUMP_HH__

#include "ast.hh"

namespace dumb
{
namespace ast
{
namespace dump
{

///
/// @brief
/// @param program
/// @param output
///
void DumpAST( const ast::Program& program, const std::string& output);

} // ! namespace dump
} // ! namespace ast
} // ! namespace dumb

#endif // ! DUMB_AST_DUMP_HH__
