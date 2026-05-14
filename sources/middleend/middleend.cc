#include <unordered_set>

#include "middleend.hh"
#include "ir.hh"
#include "ir_dump.hh"
#include "build_ssa.hh"
#include "options.hh"
#include "sccp.hh"
#include "dce.hh"
#include "lsr.hh"

namespace dumb
{

namespace
{

std::string
opt_to_str( Optimization opt)
{
    switch ( opt )
    {
        case Optimization::SCCP: return "sccp";
        case Optimization::DCE:  return "dce";
        case Optimization::LSR:  return "lsr";
        default:
        {
            throw std::runtime_error{ "Unexpected Optimization value = " +
                                      std::to_string( static_cast<int>( opt))};
        }
    }
}

} // ! anonymous namespace

std::vector<Optimization>
PipelineFromStr( const std::string& string)
{
    std::vector<Optimization> optimizations{};

    std::size_t pos = 0;
    for ( ; ; )
    {
        std::size_t start = pos;
        while ( string[pos] != '\0' &&
                string[pos] != '-' )
        {
            ++pos;
        }

        std::string opt = string.substr( start, pos - start);

        if ( opt == opt_to_str( Optimization::SCCP) )
        {
            optimizations.emplace_back( Optimization::SCCP);
        } else if ( opt == opt_to_str( Optimization::DCE) )
        {
            optimizations.emplace_back( Optimization::DCE);
        } else if ( opt == opt_to_str( Optimization::LSR) )
        {
            optimizations.emplace_back( Optimization::LSR);
        } else
        {
            throw std::runtime_error{ "Unknown optimization: " + opt};
        }

        if ( string[pos] == '\0' )
        {
            break;
        } else
        {
            ++pos;
        }
    }
    return optimizations;
}

void
RunMiddleend( ir::Program&            program,
              const MiddleendOptions& options)
{
    // Skipping main optimizations if building benchmark
    int skip_optimizations = -1;
    if ( options.benchmark_build )
    {
        const nt::Symbol *main_sym = program.Nametable().FindSymbol( "main");
        if ( main_sym == nullptr )
        {
            throw std::runtime_error{ "Program is expected to have 'main' function"};
        }
        skip_optimizations = main_sym->GetID();
    }

    build_ssa::BuildSSA( program, skip_optimizations);
    ir::dump::DumpIR( program, "build_ssa");

    std::unordered_set<Optimization> optimizations_counters{};
    for ( Optimization opt : options.pipeline )
    {
        switch ( opt )
        {
            case Optimization::SCCP:
            {
                sccp::SparseConditionalConstantPropagation( program, skip_optimizations);
                break;
            }
            case Optimization::DCE:
            {
                dce::DeadCodeElimination( program, skip_optimizations);
                break;
            }
            case Optimization::LSR:
            {
                lsr::LoopStrengthReduction( program, skip_optimizations);
                break;
            }
        }
        std::string name = opt_to_str( opt) + "_" + std::to_string( optimizations_counters.count( opt));
        ir::dump::DumpIR( program, name);
        optimizations_counters.insert( opt);
    }
}

} // ! namespace dumb
