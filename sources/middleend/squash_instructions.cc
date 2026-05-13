#include <vector>

#include "squash_instructions.hh"
#include "ssa_framework.hh"

namespace dumb
{
namespace squash_instructions
{

namespace
{

void
squash_instr_mov_pairs( ir::BasicBlock& block,
                        ir::Function&   func)
{
    for ( auto it = block.instructions.begin(); it != block.instructions.end(); )
    {
        auto next = std::next( it);
        if ( next == block.instructions.end() )
        {
            break;
        }

        // If instructions pair have form:
        //  tmp = (instruction)
        //  var = mov tmp
        // and there is no other uses of tmp in this function
        // we can replace this pair with
        //  var = (instruction)
        if ( (it->defines.type == ir::Operand::VARIABLE) &&
             (next->opcode == ir::Opcode::MOV) &&
             (next->operands[0] == it->defines) &&
             (ssa::GetUsesCount( func, it->defines) == 1) )
        {
            next->opcode   = it->opcode;
            next->operands = it->operands;
            func.RemoveVariable( it->defines.id, it->defines.value);

            it = block.instructions.erase( it);
        } else
        {
            ++it;
        }
    }
}

} // ! anonymous namespace

void
SquashInstructions( ir::Program& program)
{
    for ( ir::Function& func : program.Functions() )
    {
        for ( ir::BasicBlock& block : func.BasicBlocks() )
        {
            squash_instr_mov_pairs( block, func);
        }
    }
}

} // ! namespace squash_instructions
} // ! namespace dumb
