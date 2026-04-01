#include <iostream>
#include <string>

#include "backend.hh"
#include "frontend.hh"
#include "middleend.hh"
#include "options.hh"

#include "ir_dump.hh"
#include "utils.hh"

int
main( int         argc,
      const char *argv[])
{
    if ( argc < 2 )
    {
        throw std::runtime_error{ "Expected to have source file as a first argument"};
    }

    std::string source = argv[1];

    dumb::option::OptionsParser parser{};
    parser.AddOption<std::string>( "--test" , "-t" , "a" );
    parser.AddOption<int>        ( "--test1", "-t1", 1   );
    parser.AddOption<bool>       ( "--test2", "-t2", true);

    std::vector<std::string> args( argv + 2, argv + argc);

    parser.ParseArgs( std::move( args));

    std::string a = parser.GetOption<std::string>( "--test" );
    int b         = parser.GetOption<int>        ( "--test1");
    bool c        = parser.GetOption<bool>       ( "--test2");

    std::cout << a << " " << b << " " << c << std::endl;

    if ( argc != 3 )
    {
        std::cerr << "Unexpected parameters number" << std::endl;
        return EXIT_FAILURE;
    }

    std::string output = argv[2];

    dumb::ir::Program program_ir = dumb::RunFrontend( source);

    #if 1
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_before.svg");
    #endif

    dumb::RunMiddleend( program_ir);

    #if 1
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_after.svg");
    #endif

    std::string result = dumb::RunBackend( program_ir);

    dumb::utils::WriteTextFile( output, result);

    return 0;
}
