#include <iostream>
#include <fstream>

#include "lexer.hh"
#include "ast.hh"
#include "syntax.hh"
#include "ast_dump.hh"

std::string
readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filename);
    }

    // Перемещаем курсор в конец, чтобы узнать размер
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Оптимизированное чтение: сразу резервируем память
    std::string buffer(size, '\0');
    if (!file.read(&buffer[0], size)) {
        throw std::runtime_error("Ошибка чтения файла");
    }

    return buffer;
}

int
main( int         argc,
      const char *argv[])
{
    // TODO Read arguments and file
    if ( argc != 2 )
    {
        std::cout << "No args" << std::endl;
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    std::string source = readFile( filename);
    std::cout << source;

    // TODO End

    dumb::lexer::Lexer lexer{ source};
    auto tokens = lexer.Tokenize();

    for ( const auto &token : tokens )
    {
        std::cout << "Token (type = " << static_cast<int>( token.type)
                  << ", value = " << token.value
                  << ", line = " << token.line
                  << ", column = " << token.column << std::endl;
    }

    dumb::ast::ASTNodePtr tree = dumb::syntax::ParseSyntax( tokens, filename);
    dumb::ast::dump::DumpAST( tree, "output.svg");
}
