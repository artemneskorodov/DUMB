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

    parser.AddOption<std::string>( "--input"     , "-i" , ""                    , true );
    parser.AddOption<std::string>( "--log-flags" , "-lf", ""                    , false);
    parser.AddOption<std::string>( "--dump-dir"  , "-dd", "./"                  , false);
    parser.AddOption<bool>       ( "--benchmark" , "-b" , false                 , false);
    parser.AddOption<int>        ( "--cycles"    , "-c" , 10000                 , false);
    parser.AddOption<std::string>( "--pipeline"  , "-p" , dumb::kDefaultPipeline, false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));
    std::string source = parser.GetOption<std::string>( "--input");

    std::string dump_dir = parser.GetOption<std::string>( "--dump-dir");
    dumb::ast::dump::ConfigureDump( dump_dir);
    dumb::ir::dump::ConfigureDump( dump_dir);

    dumb::ir::Program program = dumb::RunFrontend( source);

    // Running optimizations
    std::string pipeline_str = parser.GetOption<std::string>( "--pipeline");
    auto pipeline = dumb::PipelineFromStr( pipeline_str);
    dumb::MiddleendOptions middleend_options{ pipeline, false};
    dumb::RunMiddleend( program, middleend_options);

    dumb::ir::interpreter::Run( program);
    return 0;
}
