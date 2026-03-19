#include <iostream>

#include "lexer.hh"
#include "ast.hh"
#include "syntax.hh"
#include "ast_dump.hh"
#include "utils.hh"
#include "ir.hh"
#include "emit_ir.hh"
#include "ir_dump.hh"

namespace dumb
{

ir::Program
RunFrontend( const std::string& filename)
{
    std::string source = utils::ReadTextFile( filename);
    lexer::Lexer lexer{ source};
    std::vector<lexer::Token> tokens = lexer.Tokenize();

    for ( const auto& token : tokens )
    {
        std::cout << token.line << ":" << token.column << " " << token.value << " type = " << token.type << std::endl;
    }

    ast::Program tree = syntax::ParseSyntax( tokens, filename);

    ast::dump::DumpAST( &tree, "output.svg");

    ir::Program program = emit_ir::EmitIR( &tree);
    return program;
}

} // ! namespace dumb

#if defined( BUILD_FRONTEND_SEPARATELY)

int
main( int         argc,
      const char *argv[])
{
    if ( argc != 2 )
    {
        std::cerr << "No args" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filename{ argv[1]};
}

#endif // defined( BUILD_FRONTEND_SEPARATELY)
