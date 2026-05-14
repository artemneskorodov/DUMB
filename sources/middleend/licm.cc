#include "licm.hh"

namespace dumb
{
namespace licm
{

namespace
{

ir::BasicBlockID
find_definition_block( const ir::Function& func,
                       const ir::SSAKey&   var)
{
    for ( const ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( const ir::PhiNode& phi : block.phi_nodes )
        {
            if ( phi.var == var )
            {
                return block.id;
            }
        }
        for ( const ir::Instruction& instr : block.instructions )
        {
            if ( (instr.defines.type  == ir::Operand::VARIABLE) &&
                 (instr.defines.id    == var.id) &&
                 (instr.defines.value == var.version) )
            {
                return block.id;
            }
        }
    }
    return 123123;
}

} // ! anonymous namespace

void
LoopInvariantCodeMotion( ir::Function&     func,
                         loops::LoopsInfo& loops)
{
    for ( const std::unique_ptr<loops::Loop>& loop : loops.Loops() )
    {
        ir::BasicBlock& preheader = func.GetBasicBlock( loop->preheader);

        for ( ir::BasicBlockID block_id : loop->blocks )
        {
            if ( !loop->IsBlockDirectlyInLoop( block_id) )
            {
                continue;
            }
            ir::BasicBlock& block = func.GetBasicBlock( block_id);
            auto instr_it = block.instructions.begin();
            while ( instr_it != block.instructions.end() )
            {
                if ( instr_it->HasSideEffect() )
                {
                    continue;
                }

                bool all_outside = true;
                for ( const ir::Operand& operand : instr_it->operands )
                {
                    if ( operand.type != ir::Operand::VARIABLE )
                    {
                        continue;
                    }
                    ir::BasicBlockID definition = find_definition_block( func, operand);
                    if ( loop->Contains( definition) )
                    {
                        all_outside = false;
                        break;
                    }
                }
                if ( all_outside )
                {
                    preheader.instructions.push_back( *instr_it);
                    instr_it = block.instructions.erase( instr_it);
                } else
                {
                    ++instr_it;
                }
            }
        }
    }
}

} // ! namespace licm
} // ! namespace dumb

