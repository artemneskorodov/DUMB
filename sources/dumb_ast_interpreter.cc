#include <iostream>
#include "frontend.hh"

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

    dumb::RunASTInterpreter( source);
    return 0;
}
