#ifndef DUMB_MIDDLEEND_HH__
#define DUMB_MIDDLEEND_HH__

#include "ir.hh"

namespace dumb
{

struct MiddleendOptions
{
    MiddleendOptions( bool enable_sccp,
                      bool enable_dce,
                      bool benchmark_build,
                      bool enable_lsr)
     :  enable_sccp     { enable_sccp},
        enable_dce      { enable_dce},
        benchmark_build { benchmark_build},
        enable_lsr      { enable_lsr}
    {
    }

    bool enable_sccp;
    bool enable_dce;
    bool benchmark_build;
    bool enable_lsr;

};

void RunMiddleend( ir::Program& program, const MiddleendOptions& options);

} // ! namespace dumb

#endif // ! DUMB_MIDDLEEND_HH__
