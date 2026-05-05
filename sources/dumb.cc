#include <iostream>
#include <string>

#include "backend.hh"
#include "frontend.hh"
#include "middleend.hh"
#include "options.hh"

#include "ir_dump.hh"
#include "utils.hh"
#include "logger.hh"

int
main( int         argc,
      const char *argv[])
{
    dumb::option::OptionsParser parser{};

    parser.AddOption<std::string>( "--input"      , "-i"    , ""   , true );
    parser.AddOption<std::string>( "--output"     , "-o"    , ""   , true );
    parser.AddOption<bool>       ( "--enable-sccp", "-sccp" , false, false);
    parser.AddOption<bool>       ( "--enable-dce" , "-dce"  , false, false);
    parser.AddOption<std::string>( "--log-flags"  , "-lf"   , ""   , false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));

    std::string source = parser.GetOption<std::string>( "--input");
    std::string output = parser.GetOption<std::string>( "--output");

    dumb::ir::Program program_ir = dumb::RunFrontend( source);

    #if 1
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_before.svg");
    #endif

    dumb::RunMiddleend( program_ir, parser);

    #if 1
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_after.svg");
    #endif

    std::string result = dumb::RunBackend( program_ir);

    dumb::utils::WriteTextFile( output, result);

    return EXIT_SUCCESS;
}
