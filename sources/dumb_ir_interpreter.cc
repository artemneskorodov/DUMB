#include <iostream>

#include "frontend.hh"
#include "ir_interpreter.hh"
#include "ir_dump.hh"
#include "options.hh"
#include "logger.hh"

int
main( int         argc,
      const char *argv[])
{
    dumb::option::OptionsParser parser{};

    parser.AddOption<std::string>( "--input"      , "-i"    , ""   , true );
    parser.AddOption<std::string>( "--log-flags"  , "-lf"   , ""   , false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));
    std::string source = parser.GetOption<std::string>( "--input");

    dumb::ir::Program program = dumb::RunFrontend( source);

    // Dumping IR
    dumb::ir::dump::DumpIR( program, "ir_dump");

    dumb::ir::interpreter::Run( program);
    return 0;
}
