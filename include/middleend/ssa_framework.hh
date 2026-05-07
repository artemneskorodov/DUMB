#ifndef DUMB_SSA_FRAMEWORK_HH__
#define DUMB_SSA_FRAMEWORK_HH__

#include <vector>

#include "ir.hh"

namespace dumb
{
namespace ssa
{

///
/// @brief         Find all PhiNode users of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key SSA variable to search for.
/// @return        Object to iterate.
///
std::vector<const ir::PhiNode *>
GetPhiUsers( const ir::Function& func,
             const ir::SSAKey&   ssa_key);

///
/// @brief         Find all Instruction users of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key SSA variable to search for.
/// @return        Object to iterate.
///
std::vector<const ir::Instruction *>
GetInstrUsers( const ir::Function& func,
               const ir::SSAKey&   ssa_key);

///
/// @brief         Find all BasicBlockTerminators users of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key SSA variable to search for.
/// @return        Object to iterate.
///
std::vector<const ir::BasicBlockTerminator *>
GetTermUsers( const ir::Function& func,
              const ir::SSAKey&   ssa_key);

///
/// @brief         Find all uses of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key Variable to search for.
/// @return        Object to iterate through uses
///
std::vector<ir::Operand *>
GetUses( ir::Function&     func,
         const ir::SSAKey& ssa_key);

} // ! namespace ssa
} // ! namespace dumb

#endif // ! DUMB_SSA_FRAMEWORK_HH__
