#ifndef DUMB_IR_DUMP_HH__
#define DUMB_IR_DUMP_HH__

#include "ir.hh"

namespace dumb
{
namespace ir
{
namespace dump
{

///
/// @brief Configure IR dump directory.
/// @param dir Directory to save IR dumps to.
///
void ConfigureDump( const std::string& dir);

///
/// @brief Dump IR if IR dumping is enabled
/// @param program IR.
/// @param output Name of dump.
///
void DumpIR( const Program& program, const std::string& output);

} // ! namespace dump
} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_DUMP_HH__
