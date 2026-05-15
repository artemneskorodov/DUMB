#ifndef DUMB_MIDDLEEND_HH__
#define DUMB_MIDDLEEND_HH__

#include <vector>

#include "ir.hh"

namespace dumb
{

enum class Optimization
{
    SCCP,
    DCE,
    LSR,
    ISIM,
};

inline std::string kDefaultPipeline = "sccp-dce-isim-lsr-sccp-dce";

struct MiddleendOptions
{
    MiddleendOptions( std::vector<Optimization> pipeline,
                      bool                      benchmark_build)
     :  pipeline        { std::move( pipeline)},
        benchmark_build { benchmark_build}
    {
    }

    std::vector<Optimization> pipeline;
    bool                      benchmark_build;

};

std::vector<Optimization> PipelineFromStr( const std::string& string);

void RunMiddleend( ir::Program& program, const MiddleendOptions& options);

} // ! namespace dumb

#endif // ! DUMB_MIDDLEEND_HH__
