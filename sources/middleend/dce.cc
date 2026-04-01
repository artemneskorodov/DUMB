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
get_uses( const ir::SSAKey&   var,
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

std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash>
get_uses_counters( const ir::Function& function)
{
    std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash> result{};

    for ( const ir::BasicBlock& block : function.BasicBlocks() )
    {
        for ( const ir::Instruction& instr : block.instructions )
        {
            nt::SymbolID id = instr.defines.id;
            int version = instr.defines.value;
            if ( (instr.defines.type == ir::Operand::VARIABLE) &&
                 (result.find( ir::SSAKey{ id, version}) == result.end()) )
            {
                result[ir::SSAKey{ instr.defines.id, instr.defines.value}] = 0;
            }
            for ( const ir::Operand& op : instr.operands )
            {
                if ( op.type == ir::Operand::VARIABLE )
                {
                    if ( result.find( ir::SSAKey{ op.id, op.value}) == result.end() )
                    {
                        result[ir::SSAKey{ op.id, op.value}] = 0;
                    }
                    ++result[ir::SSAKey{ op.id, op.value}];
                }
            }
        }

        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            if ( result.find( ir::SSAKey{ phi.var_id, phi.version}) == result.end() )
            {
                result[ir::SSAKey{ phi.var_id, phi.version}] = 0;
            }
            for ( auto& [pred, op] : phi.mapping )
            {
                if ( op.type  == ir::Operand::VARIABLE )
                {
                    if ( result.find( ir::SSAKey{ op.id, op.value}) == result.end() )
                    {
                        result[ir::SSAKey{ op.id, op.value}] = 0;
                    }
                    ++result[ir::SSAKey{ op.id, op.value}];
                }
            }
        }
    }

    return result;
}

void
remove_def( const ir::SSAKey& var,
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
        std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash> counters = get_uses_counters( func);

        bool changed = true;

        while ( changed )
        {
            changed = false;

            std::vector<ir::SSAKey> zeros{};
            for ( auto& [value, count] : counters )
            {
                if ( count == 0 )
                {
                    zeros.emplace_back( value.id, value.version);
                }
            }

            for ( ir::SSAKey& value : zeros )
            {
                for ( ir::SSAKey& use : get_uses( value, func) )
                {
                    if ( counters.find( use) != counters.end() )
                    {
                        counters[use] -= 1;
                    }
                }

                remove_def( value, func);
                counters.erase( value);
                changed = true;
            }
        }
    }
}

} // ! namespace dce
} // ! namespace dumb
