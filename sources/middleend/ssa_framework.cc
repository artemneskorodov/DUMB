#include <vector>

#include "ssa_framework.hh"
#include "ir.hh"

namespace dumb
{
namespace ssa
{

namespace
{

bool
is_same_var( const ir::SSAKey&  ssa_key,
             const ir::Operand& operand)
{
    return (operand.type  == ir::Operand::VARIABLE) &&
           (operand.id    == ssa_key.id) &&
           (operand.value == ssa_key.version);
}

} // ! anonymous namespace

std::vector<const ir::PhiNode *>
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
                if ( is_same_var( ssa_key, operand) )
                {
                    users.emplace_back( &phi);
                }
            }
        }
    }
    return users;
}

std::vector<const ir::Instruction *>
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
                if ( is_same_var( ssa_key, operand) )
                {
                    users.emplace_back( &instr);
                }
            }
        }
    }
    return users;
}

std::vector<const ir::BasicBlockTerminator *>
GetTermUsers( const ir::Function& func,
              const ir::SSAKey&   ssa_key)
{
    std::vector<const ir::BasicBlockTerminator *> users{};

    for ( const ir::BasicBlock& block : func.BasicBlocks() )
    {
        const ir::BasicBlockTerminator& term = block.terminator;
        if ( is_same_var( ssa_key, term.left) )
        {
            users.emplace_back( &term);
        } else if ( is_same_var( ssa_key, term.right) )
        {
            users.emplace_back( &term);
        }
    }
    return users;
}

std::vector<ir::Operand *>
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
                if ( is_same_var( ssa_key, operand) )
                {
                    uses.emplace_back( &operand);
                }
            }
        }
        for ( ir::Instruction& instr : block.instructions )
        {
            for ( ir::Operand& operand : instr.operands )
            {
                if ( is_same_var( ssa_key, operand) )
                {
                    uses.emplace_back( &operand);
                }
            }
        }

        ir::BasicBlockTerminator& term = block.terminator;
        if ( is_same_var( ssa_key, term.left) )
        {
            uses.emplace_back( &term.left);
        }
        if ( is_same_var( ssa_key, term.right) )
        {
            uses.emplace_back( &term.right);
        }
    }
    return uses;
}

} // ! namespace ssa
} // ! namespace dumb
