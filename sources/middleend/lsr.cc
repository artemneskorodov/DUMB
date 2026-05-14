#include "lsr.hh"
#include "ssa_framework.hh"
#include "iv_search.hh"

namespace dumb
{
namespace lsr
{

namespace
{

ir::Operand
get_tmp( ir::Program&  program,
         ir::Function& func,
         std::size_t&  counter)
{
    std::string name = "__tmp_lsr_" + std::to_string( counter);
    ++counter;
    nt::SymbolID id = program.Nametable().AddSymbol( name, nt::SymbolType::LOCAL_VARIABLE);
    func.AddVariable( id, 0);
    return ir::Operand{ ir::Operand::VARIABLE, 0, id};
}

void
loop_strength_reduction( ir::Program&                 program,
                         ir::Function&                func,
                         const iv::DerivedIndVarList& derived_inductions)
{
    std::size_t tmp_counter = 0;

    for ( const iv::DerivedInductionVar& derived_ind : derived_inductions )
    {
        loops::Loop *loop = derived_ind.base->loop;

        ir::BasicBlock& preheader = func.GetBasicBlock( loop->preheader);
        ir::BasicBlock& header    = func.GetBasicBlock( loop->header);
        ir::BasicBlock& latch     = func.GetBasicBlock( loop->latch);

        // Replacing derived induction variable in form: x = mul * i + offset,
        // where i is basic induction variable with i_init and i_step:
        // i_n = i_init + n * i_step
        // x_n = mul * (i_init + n * i_step) + offset =
        //     = (mul * i_init + offset) + n * (mul * i_step)
        // x_n is equivalent to induction variable x with:
        //     init = (mul * i_init + offset),
        //     offset = (mul * i_step)

        // Evaluating tmp = mul * i_init in preheader
        ir::Operand init_tmp = get_tmp( program, func, tmp_counter);
        preheader.instructions.emplace_back(
            ir::Instruction{
                ir::Opcode::MUL,
                init_tmp,
                std::vector<ir::Operand>{
                    ir::Operand{ ir::Operand::IMMEDIATE, derived_ind.multiplier},
                    derived_ind.base->init
                }
            }
        );

        // Evaluating init_tmp + offset which is equal to (mul * i_init + offset) = init
        ir::Operand init = get_tmp( program, func, tmp_counter);
        preheader.instructions.emplace_back(
            ir::Instruction{
                ir::Opcode::ADD,
                init,
                std::vector<ir::Operand>{
                    init_tmp,
                    ir::Operand{ ir::Operand::IMMEDIATE, derived_ind.offset}
                }
            }
        );

        // Evaluating offset = (mul * i_step)
        ir::Operand step = get_tmp( program, func, tmp_counter);
        preheader.instructions.emplace_back(
            ir::Instruction{
                ir::Opcode::MUL,
                step,
                std::vector<ir::Operand>{
                    ir::Operand{ ir::Operand::IMMEDIATE, derived_ind.multiplier},
                    derived_ind.base->step
                }
            }
        );

        ir::Instruction *instr;
        if ( derived_ind.add == nullptr )
        {
            instr = derived_ind.mul;
        } else
        {
            instr = derived_ind.add;
        }

        // Replace definition of x in root instruction ( add, or mul if there is no add )
        // with x = x0 + step
        // DON'T Remove definition of not root instruction if it occurs ( there are two derived
        //  inductions for each x = a * i + b: x1 = a * i and x2 = a * i + b), it is not correct
        //  to delete x1 as it can be used. Replace only one instruction and other if it is unused
        //  will be deleted by DCE
        // Add x0 = PHI(init_tmp, x) to header

        // Adding x_tmp = PHI(BB_PREHEADER:init_tmp, BB_LATCH:x)
        // x here is derived inductive variable itself
        ir::Operand phi_tmp = get_tmp( program, func, tmp_counter);

        header.phi_nodes.emplace_back( ir::PhiNode{ phi_tmp.id});
        ir::PhiNode& phi = header.phi_nodes.back();
        phi.mapping[preheader.id] = init;
        phi.mapping[latch.id]     = instr->defines;

        // Replacing x = tmp + CONST or x = CONST * i with x = x_tmp + step
        instr->opcode = ir::Opcode::ADD;
        instr->operands[0] = phi_tmp;
        instr->operands[1] = step;
    }
}

} // ! anonymous namespace

void
LoopStrengthReduction( ir::Program& program,
                       nt::SymbolID skip_optimizations)
{
    for ( ir::Function& func : program.Functions() )
    {
        if ( func.Id() == skip_optimizations )
        {
            continue;
        }

        graph::Graph<ir::BasicBlockID> cfg = ssa::BuildCFG( func);
        cfg.BuildDominatorsTable();

        // Creating loops tree
        loops::LoopsInfo loops_info{ cfg};
        LOGGER(LOOP_ANALYSIS) << loops_info.ToStr();

        // Creating basic induction variables list
        iv::BasicIndVarList basic_inductions = iv::GetInductiveVariables( func, loops_info);
        for ( const iv::BasicInductionVar& basic_ind : basic_inductions )
        {
            LOGGER(LOOP_ANALYSIS) << "Basic induction: " << basic_ind.ToStr();
        }

        iv::DerivedIndVarList derived_inductions = iv::GetDerivedInductiveVariables( func,
                                                                                     loops_info,
                                                                                     basic_inductions);
        for ( const iv::DerivedInductionVar& derived_ind : derived_inductions )
        {
            LOGGER(LOOP_ANALYSIS) << "Derived induction: " << derived_ind.ToStr();
        }

        loop_strength_reduction( program, func, derived_inductions);
    }
}

} // ! namespace lsr
} // ! namespace dumb
