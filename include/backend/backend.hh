#ifndef DUMB_BACKEND_HH__
#define DUMB_BACKEND_HH__

#include "ir.hh"

namespace dumb
{

struct BackendOptions
{
    BackendOptions( bool build_benchmark_asm)
     :  build_benchmark_asm{ build_benchmark_asm}
    {
    }

    bool build_benchmark_asm;

};

std::string RunBackend( const ir::Program& program, const BackendOptions& options);

} // ! namespace dumb

#endif // ! DUMB_BACKEND_HH__
