#include <iostream>

#include "frontend.hh"
#include "ir_interpreter.hh"
#include "ir_dump.hh"
#include "options.hh"
#include "logger.hh"
#include "middleend.hh"
#include "ast_dump.hh"

int
main( int         argc,
      const char *argv[])
{
    dumb::option::OptionsParser parser{};

    parser.AddOption<std::string>( "--input"      , "-i"    , ""   , true );
    parser.AddOption<bool>       ( "--enable-sccp", "-sccp" , false, false);
    parser.AddOption<bool>       ( "--enable-dce" , "-dce"  , false, false);
    parser.AddOption<std::string>( "--log-flags"  , "-lf"   , ""   , false);
    parser.AddOption<std::string>( "--dump-dir"   , "-dd"   , "./" , false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));
    std::string source = parser.GetOption<std::string>( "--input");

    std::string dump_dir = parser.GetOption<std::string>( "--dump-dir");
    dumb::ast::dump::ConfigureDump( dump_dir);
    dumb::ir::dump::ConfigureDump( dump_dir);

    dumb::ir::Program program = dumb::RunFrontend( source);

    // Dumping IR before middleend
    dumb::ir::dump::DumpIR( program, "ir_dump_before_opt");

    // Running IR optimizations
    dumb::RunMiddleend( program, parser);

    // Dumping IR after middleend
    dumb::ir::dump::DumpIR( program, "ir_dump_after_opt");

    dumb::ir::interpreter::Run( program);
    return 0;
}
