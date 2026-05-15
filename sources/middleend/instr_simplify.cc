#include "instr_simplify.hh"
#include "ssa_framework.hh"

namespace dumb
{
namespace instr_simplify
{

namespace
{

bool
simplify_mul( ir::Instruction& instr)
{
    if ( instr.operands[0].type  == ir::Operand::IMMEDIATE &&
         instr.operands[0].value == 0 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands.resize( 1);
        instr.operands[0] = ir::Operand{ ir::Operand::IMMEDIATE, 0};
        return true;
    }
    if ( instr.operands[1].type  == ir::Operand::IMMEDIATE &&
         instr.operands[1].value == 0 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands.resize( 1);
        instr.operands[0] = ir::Operand{ ir::Operand::IMMEDIATE, 0};
        return true;
    }
    if ( instr.operands[0].type  == ir::Operand::IMMEDIATE &&
         instr.operands[0].value == 1 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands[0] = instr.operands[1];
        instr.operands.resize( 1);
        return true;
    }
    if ( instr.operands[1].type  == ir::Operand::IMMEDIATE &&
         instr.operands[1].value == 1 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands[0] = instr.operands[0];
        instr.operands.resize( 1);
        return true;
    }
    return false;
}

bool
simplify_add_sub( ir::Instruction& instr)
{
    if ( instr.operands[0].type  == ir::Operand::IMMEDIATE &&
         instr.operands[0].value == 0 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands[0] = instr.operands[1];
        instr.operands.resize( 1);
        return true;
    }
    if ( instr.operands[1].type  == ir::Operand::IMMEDIATE &&
         instr.operands[1].value == 0 )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands[0] = instr.operands[0];
        instr.operands.resize( 1);
        return true;
    }
    return false;
}

bool
simplify_add( ir::Instruction& instr)
{
    return simplify_add_sub( instr);
}

bool
simplify_sub( ir::Instruction& instr)
{
    if ( simplify_add_sub( instr) )
    {
        return true;
    }

    if ( instr.operands[0] == instr.operands[1] )
    {
        instr.opcode = ir::Opcode::MOV;
        instr.operands[0] = ir::Operand{ ir::Operand::IMMEDIATE, 0};
        instr.operands.resize( 1);
        return true;
    }
    return false;
}

void
squash_movs( ir::Function& func)
{
    for ( ir::BasicBlock& block : func.BasicBlocks() )
    {
        auto instr_it = block.instructions.begin();
        while ( instr_it != block.instructions.end() )
        {
            auto next = std::next( instr_it);
            if ( next == block.instructions.end() )
            {
                break;
            }

            //
            // Trying to get pattern: tmp = (instruction); var = mov(tmp);
            // In this case if tmp is not used anywhere else, this can be squashed to var = (instruction)
            //
            if ( instr_it->defines.type != ir::Operand::VARIABLE ||
                 ssa::GetUsesCount( func, instr_it->defines) != 1 )
            {
                ++instr_it;
                continue;
            }

            auto instr_uses = ssa::GetInstrUsers( func, instr_it->defines);
            if ( instr_uses.size() == 0 )
            {
                ++instr_it;
                continue;
            }

            ir::Instruction *use = const_cast<ir::Instruction *>( instr_uses.front());
            if ( use->opcode != ir::Opcode::MOV )
            {
                ++instr_it;
                continue;
            }

            use->opcode = instr_it->opcode;
            use->operands = instr_it->operands;
            func.RemoveVariable( instr_it->defines.id, instr_it->defines.value);

            instr_it = block.instructions.erase( instr_it);
        }
    }
}

} // ! anonymous namespace

void
InstructionsSimplification( ir::Program& program,
                            nt::SymbolID skip_optimizations)
{
    for ( ir::Function& func : program.Functions() )
    {
        if ( func.Id() == skip_optimizations )
        {
            continue;
        }
        for ( ir::BasicBlock& block : func.BasicBlocks() )
        {
            auto instr_it = block.instructions.begin();
            while ( instr_it != block.instructions.end() )
            {
                if ( instr_it->opcode == ir::Opcode::MUL )
                {
                    simplify_mul( *instr_it);
                } else if ( instr_it->opcode == ir::Opcode::ADD )
                {
                    simplify_add( *instr_it);
                } else if ( instr_it->opcode == ir::Opcode::SUB )
                {
                    simplify_sub( *instr_it);
                }

                ++instr_it;
            }
        }
        squash_movs( func);
    }
}

} // ! namespace instr_simplify
} // ! namespace dumb
