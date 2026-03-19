#include <iostream>

#include "backend.hh"
#include "frontend.hh"
#include "ir.hh"

#include "ir_dump.hh"

int
main( int argc,
      const char *argv[])
{
    if ( argc != 2 )
    {
        std::cerr << "No args" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filename{ argv[1]};

    dumb::ir::Program program_ir = dumb::RunFrontend( filename);

    dumb::ir_dump::DumpIR( &program_ir);

    std::string result = dumb::RunBackend( &program_ir);
    std::cout << result;
    return 0;
}
