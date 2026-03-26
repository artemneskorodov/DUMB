#include <iostream>
#include "frontend.hh"
#include "ir_interpreter.hh"
#include "ir_dump.hh"

int
main( int         argc,
      const char *argv[])
{
    if ( argc != 2 )
    {
        std::cerr << "Unexpected parameters number" << std::endl;
        return EXIT_FAILURE;
    }

    std::string source = argv[1];

    dumb::ir::Program program = dumb::RunFrontend( source);
    #if 0
    dumb::ir_dump::DumpIR( program);
    #endif
    dumb::ir::interpreter::Run( program);
    return 0;
}
