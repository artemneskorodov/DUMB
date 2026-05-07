#ifndef DUMB_BACKEND_HH__
#define DUMB_BACKEND_HH__

#include "ir.hh"

namespace dumb
{

struct BackendOptions
{
    BackendOptions( bool build_benchmark_asm,
                    int  benchmarks_cycles)
     :  build_benchmark_asm{ build_benchmark_asm},
        benchmarks_cycles{ benchmarks_cycles}
    {
    }

    bool build_benchmark_asm;
    int benchmarks_cycles;

};

std::string RunBackend( const ir::Program& program, const BackendOptions& options);

} // ! namespace dumb

#endif // ! DUMB_BACKEND_HH__
