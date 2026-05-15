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

    parser.AddOption<std::string>( "--input"     , "-i"  , ""                    , true );
    parser.AddOption<std::string>( "--output"    , "-o"  , ""                    , true );
    parser.AddOption<std::string>( "--log-flags" , "-lf" , ""                    , false);
    parser.AddOption<std::string>( "--dump-dir"  , "-dd" , "./"                  , false);
    parser.AddOption<bool>       ( "--benchmark" , "-b"  , false                 , false);
    parser.AddOption<int>        ( "--cycles"    , "-c"  , 10000                 , false);
    parser.AddOption<std::string>( "--pipeline"  , "-p"  , dumb::kDefaultPipeline, false);
    parser.AddOption<bool>       ( "--polish"    , "-pol", false                 , false);

    std::vector<std::string> args( argv + 1, argv + argc);

    parser.ParseArgs( std::move( args));

    dumb::logger::ConfigureLogger( parser.GetOption<std::string>( "--log-flags"));

    // Configuring dump directories for AST and IR graphs
    std::string dump_dir = parser.GetOption<std::string>( "--dump-dir");
    dumb::ast::dump::ConfigureDump( dump_dir);
    dumb::ir::dump::ConfigureDump( dump_dir);

    std::string source = parser.GetOption<std::string>( "--input");
    std::string output = parser.GetOption<std::string>( "--output");

    if ( parser.GetOption<bool>( "--polish") )
    {
        std::string polish = dumb::GeneratePolish( source);
        dumb::utils::WriteTextFile( output, polish);
        return EXIT_SUCCESS;
    }

    dumb::ir::Program program_ir = dumb::RunFrontend( source);

    // Running optimizations
    std::string pipeline_str = parser.GetOption<std::string>( "--pipeline");
    auto pipeline = dumb::PipelineFromStr( pipeline_str);
    dumb::MiddleendOptions middleend_options{ pipeline,
                                              parser.GetOption<bool>( "--benchmark")};
    dumb::RunMiddleend( program_ir, middleend_options);

    // Running backend
    dumb::BackendOptions backend_options{ parser.GetOption<bool>( "--benchmark"),
                                          parser.GetOption<int> ( "--cycles")};
    std::string result = dumb::RunBackend( program_ir, backend_options);

    dumb::utils::WriteTextFile( output, result);

    return EXIT_SUCCESS;
}
