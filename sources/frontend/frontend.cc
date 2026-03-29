#include <iostream>

#include "lexer.hh"
#include "ast.hh"
#include "syntax.hh"
#include "ast_dump.hh"
#include "utils.hh"
#include "ir.hh"
#include "emit_ir.hh"
#include "emit_ir.hh"
#include "ast_interpreter.hh"

namespace dumb
{

ir::Program
RunFrontend( const std::string& filename)
{
    std::string source = utils::ReadTextFile( filename);
    lexer::Lexer lexer{ source};
    std::vector<lexer::Token> tokens = lexer.Tokenize();

    ast::Program tree = syntax::ParseSyntax( tokens, filename);

    #if 1
    ast::dump::DumpAST( tree, "dump_ast.svg");
    #endif

    ir::Program program = emit_ir::EmitIR( tree);
    return program;
}

void
RunASTInterpreter( const std::string& filename)
{
    std::string source = utils::ReadTextFile( filename);
    lexer::Lexer lexer{ source};
    std::vector<lexer::Token> tokens = lexer.Tokenize();

    ast::Program tree = syntax::ParseSyntax( tokens, filename);

    ast::interpreter::Run( tree);
}

} // ! namespace dumb
