#include <iostream>

#include "backend.hh"
#include "frontend.hh"
#include "middleend.hh"

#include "ir_dump.hh"
#include "utils.hh"

int
main( int         argc,
      const char *argv[])
{
    if ( argc != 3 )
    {
        std::cerr << "Unexpected parameters number" << std::endl;
        return EXIT_FAILURE;
    }

    std::string source = argv[1];
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
