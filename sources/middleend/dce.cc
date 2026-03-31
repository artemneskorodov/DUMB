#include <map>
#include <iostream>

#include "ir.hh"

namespace dumb
{
namespace dce
{

namespace
{

std::vector<ir::SSAKey>
GetUses( const ir::SSAKey&   var,
         const ir::Function& function)
{
    std::vector<ir::SSAKey> result{};

    for ( const ir::BasicBlock& block : function.BasicBlocks() )
    {
        for ( const ir::Instruction& instr : block.instructions )
        {
            if ( instr.defines.type != ir::Operand::VARIABLE )
            {
                continue;
            }
            for ( const ir::Operand& op : instr.operands )
            {
                if ( (op.type  == ir::Operand::VARIABLE) &&
                     (op.id    == var.id) &&
                     (op.value == var.version) )
                {
                    result.emplace_back( instr.defines.id,
                                         instr.defines.value);
                }
            }
        }

        // phi
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            for ( auto& [pred, op] : phi.mapping )
            {
                if ( (op.type  == ir::Operand::VARIABLE) &&
                     (op.id    == var.id) &&
                     (op.value == var.version) )
                {
                    result.emplace_back( phi.var_id, phi.version);
                }
            }
        }
    }

    return result;
}

std::size_t
GetUsesCount( const ir::Function& function,
              const ir::SSAKey& var)
{
    std::size_t result = 0;

    for ( const ir::BasicBlock& block : function.BasicBlocks() )
    {
        for ( const ir::Instruction& instr : block.instructions )
        {
            for ( const ir::Operand& op : instr.operands )
            {
                if ( (op.type  == ir::Operand::VARIABLE) &&
                     (op.id    == var.id) &&
                     (op.value == var.version) )
                {
                    ++result;
                }
            }
        }

        // phi
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            for ( auto& [pred, op] : phi.mapping )
            {
                if ( (op.type  == ir::Operand::VARIABLE) &&
                     (op.id    == var.id) &&
                     (op.value == var.version) )
                {
                    ++result;
                }
            }
        }
    }

    return result;
}

std::vector<ir::SSAKey>
GetAllValues( const ir::Function& function)
{
    std::vector<ir::SSAKey> values;

    for ( const ir::BasicBlock& block : function.BasicBlocks() )
    {
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            values.emplace_back(phi.var_id, phi.version);
        }

        for ( const ir::Instruction& instr : block.instructions )
        {
            if ( instr.defines.type == ir::Operand::VARIABLE )
            {
                values.emplace_back( instr.defines.id,
                                     instr.defines.value);
            }
        }
    }

    return values;
}

void
RemoveDef( const ir::SSAKey& var,
           ir::Function&     function)
{
    for ( ir::BasicBlock& block : function.BasicBlocks() )
    {
        block.instructions.remove_if(
            [var](const ir::Instruction& instr)
            {
                return (instr.defines.type  == ir::Operand::VARIABLE) &&
                       (instr.defines.id    == var.id) &&
                       (instr.defines.value == var.version);
            }
        );

        block.phi_nodes.remove_if(
            [var](const ir::PhiNode& phi)
            {
                return (phi.var_id  == var.id) &&
                       (phi.version == var.version);
            }
        );
    }
    function.RemoveVariable( var.id, var.version);
}

} // anonymous namespace

void
DeadCodeElimination( ir::Program& ir)
{
    for ( ir::Function& func : ir.Functions() )
    {
        std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash> counters{};

        for ( ir::SSAKey& value : GetAllValues( func) )
        {
            counters[value] = 0;

            counters[value] += GetUsesCount( func, value);
        }

        bool changed = true;

        while ( changed )
        {
            changed = false;

            std::vector<ir::SSAKey> zeros;

            for ( auto& [value, count] : counters )
            {
                if ( count == 0 )
                {
                    zeros.push_back( value);
                }
            }

            for ( ir::SSAKey& value : zeros )
            {
                for ( ir::SSAKey& use : GetUses( value, func) )
                {
                    if ( counters.find( use) != counters.end() )
                    {
                        counters[use] -= 1;
                    }
                }

                RemoveDef( value, func);
                counters.erase( value);
                changed = true;
            }
        }
    }
}

} // ! namespace dce
} // ! namespace dumb
