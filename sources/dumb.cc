#include <iostream>
#include <string>

#include "backend.hh"
#include "frontend.hh"
#include "ast_dump.hh"
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
    parser.AddOption<std::string>( "--dump-dir"   , "-dd"   , "./" , false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));

    // Configuring dump directories for AST and IR graphs
    std::string dump_dir = parser.GetOption<std::string>( "--dump-dir");
    dumb::ast::dump::ConfigureDump( dump_dir);
    dumb::ir::dump::ConfigureDump( dump_dir);

    std::string source = parser.GetOption<std::string>( "--input");
    std::string output = parser.GetOption<std::string>( "--output");

    dumb::ir::Program program_ir = dumb::RunFrontend( source);

    // Dumping IR before middleend
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_before_middleend");

    // Running optimizations
    dumb::RunMiddleend( program_ir, parser);

    // Dumping IR after middleend
    dumb::ir::dump::DumpIR( program_ir, "ir_dump_after_middleend");

    std::string result = dumb::RunBackend( program_ir);

    dumb::utils::WriteTextFile( output, result);

    return EXIT_SUCCESS;
}
