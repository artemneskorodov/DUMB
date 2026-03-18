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

ast::ASTNodePtr
RunFrontend( const std::string& filename)
{
    std::string source = utils::ReadTextFile( filename);
    lexer::Lexer lexer{ source};
    std::vector<lexer::Token> tokens = lexer.Tokenize();

    for ( const auto& token : tokens )
    {
        std::cout << token.line << ":" << token.column << " " << token.value << " type = " << token.type << std::endl;
    }

    ast::ASTNodePtr tree = syntax::ParseSyntax( tokens, filename);
    ast::dump::DumpAST( tree, "output.svg");

    ir::Program program = emit_ir::EmitIR( std::move( tree));

    ir_dump::DumpIR( &program);

    return tree;
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

    dumb::ast::ASTNodePtr tree = dumb::RunFrontend( filename);
}

#endif // defined( BUILD_FRONTEND_SEPARATELY)
