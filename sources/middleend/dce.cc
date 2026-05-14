#include <map>
#include <iostream>

#include "ir.hh"
#include "logger.hh"

namespace dumb
{
namespace dce
{

namespace
{

///
/// @brief          Decrement counters for all operands of definition of variable.
/// @param func     Function to look for definition.
/// @param var      Variable to search for.
/// @param counters Counters of uses of all variables - state of IR
/// @return         true if definition of variable can be deleted
///
bool
decrement_uses_of_definition( const ir::Function&                                  func,
                              const ir::SSAKey&                                    var,
                              std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash>& counters)
{
    for ( const ir::BasicBlock& block : func.BasicBlocks() )
    {
        // Searching for definition in instructions
        for ( const ir::Instruction& instr : block.instructions )
        {
            if ( (instr.defines.type  == ir::Operand::VARIABLE) &&
                 (instr.defines.id    == var.id) &&
                 (instr.defines.value == var.version) )
            {
                if ( instr.HasSideEffect() )
                {
                    return false;
                }
                // Decrementing uses counters for every operand
                for ( const ir::Operand& operand : instr.operands )
                {
                    if ( operand.type == ir::Operand::VARIABLE )
                    {
                        ir::SSAKey key{ operand.id, operand.value};
                        --counters[key];
                    }
                }
                return true;
            }
        }

        // Searching for definition in phi nodes
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            if ( phi.var == var )
            {
                // Decrementing uses counters for every operand
                for ( const auto& [bb_id, operand] : phi.mapping )
                {
                    if ( operand.type == ir::Operand::VARIABLE )
                    {
                        ir::SSAKey key{ operand.id, operand.value};
                        --counters[key];
                    }
                }
                return true;
            }
        }
    }

    // TODO can reach this?
    return true;
}

std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash>
get_uses_counters( const ir::Function& function)
{
    std::unordered_map<ir::SSAKey, int, ir::SSAKeyHash> result{};

    auto increase_counter = [&result]( const ir::Operand& operand )
                            {
                                if ( operand.type == ir::Operand::VARIABLE )
                                {
                                    ir::SSAKey key{ operand.id, operand.value};
                                    if ( result.find( key) == result.end() )
                                    {
                                        result[key] = 0;
                                    }
                                    ++result[key];
                                }
                            };

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
                increase_counter( op);
            }
        }

        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            if ( result.find( phi.var) == result.end() )
            {
                result[phi.var] = 0;
            }
            for ( auto& [pred, op] : phi.mapping )
            {
                increase_counter( op);
            }
        }

        increase_counter( block.terminator.left);
        increase_counter( block.terminator.right);
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
                return (phi.var.id    == var.id) &&
                       (phi.var.value == var.version);
            }
        );
    }
    function.RemoveVariable( var.id, var.version);
}

} // anonymous namespace

void
DeadCodeElimination( ir::Program& ir,
                     int          skip_func)
{
    for ( ir::Function& func : ir.Functions() )
    {
        if ( func.Id() == skip_func )
        {
            continue;
        }

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
                if ( decrement_uses_of_definition( func, value, counters) )
                {
                    remove_def( value, func);
                    counters.erase( value);
                    changed = true;
                }
            }
        }
    }
}

} // ! namespace dce
} // ! namespace dumb
