#ifndef DUMB_SSA_FRAMEWORK_HH__
#define DUMB_SSA_FRAMEWORK_HH__

#include <vector>

#include "ir.hh"

namespace dumb
{
namespace ssa
{

namespace detail
{

bool
is_same_var( const ir::SSAKey&  ssa_key,
             const ir::Operand& operand)
{
    return (operand.type  == ir::Operand::VARIABLE) &&
           (operand.id    == ssa_key.id) &&
           (operand.value == ssa_key.version);
}

} // ! namespace detail

///
/// @brief         Find all PhiNode users of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key SSA variable to search for.
/// @return        Object to iterate.
///
inline std::vector<const ir::PhiNode *>
GetPhiUsers( const ir::Function& func,
             const ir::SSAKey&   ssa_key)
{
    std::vector<const ir::PhiNode *> users{};

    for ( const ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            for ( const auto& [pred_id, operand] : phi.mapping )
            {
                if ( detail::is_same_var( ssa_key, operand) )
                {
                    users.emplace_back( &phi);
                }
            }
        }
    }
    return users;
}

///
/// @brief         Find all Instruction users of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key SSA variable to search for.
/// @return        Object to iterate.
///
inline std::vector<const ir::Instruction *>
GetInstrUsers( const ir::Function& func,
               const ir::SSAKey&   ssa_key)
{
    std::vector<const ir::Instruction *> users{};

    for ( const ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( const ir::Instruction& instr : block.instructions )
        {
            for ( const ir::Operand& operand : instr.operands )
            {
                if ( detail::is_same_var( ssa_key, operand) )
                {
                    users.emplace_back( &instr);
                }
            }
        }
    }
    return users;
}

///
/// @brief         Find all uses of SSA variable.
/// @param func    Function to look for uses.
/// @param ssa_key Variable to search for.
/// @return        Object to iterate through uses
///
inline std::vector<ir::Operand *>
GetUses( ir::Function&     func,
         const ir::SSAKey& ssa_key)
{
    std::vector<ir::Operand *> uses{};

    for ( ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( ir::PhiNode& phi : block.phi_nodes )
        {
            for ( auto& [pred_id, operand] : phi.mapping )
            {
                if ( detail::is_same_var( ssa_key, operand) )
                {
                    uses.emplace_back( &operand);
                }
            }
        }
        for ( ir::Instruction& instr : block.instructions )
        {
            for ( ir::Operand& operand : instr.operands )
            {
                if ( detail::is_same_var( ssa_key, operand) )
                {
                    uses.emplace_back( &operand);
                }
            }
        }

        ir::BasicBlockTerminator& term = block.terminator;
        if ( detail::is_same_var( ssa_key, term.left) )
        {
            uses.emplace_back( &term.left);
        }
        if ( detail::is_same_var( ssa_key, term.right) )
        {
            uses.emplace_back( &term.right);
        }
    }
    return uses;
}

} // ! namespace ssa
} // ! namespace dumb

#endif // ! DUMB_SSA_FRAMEWORK_HH__
