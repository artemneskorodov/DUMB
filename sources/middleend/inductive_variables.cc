#include <optional>

#include "inductive_variables.hh"
#include "logger.hh"

namespace dumb
{
namespace iv
{

namespace
{

bool
is_same_var( const ir::Operand& operand,
             nt::SymbolID       id,
             int                version)
{
    return ( (operand.type  == ir::Operand::VARIABLE) &&
             (operand.id    == id) &&
             (operand.value == version) );
}

ir::Instruction*
find_definition( ir::Function& func,
                 nt::SymbolID  id,
                 int           version)
{
    for ( ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( ir::Instruction& instr : block.instructions )
        {
            if ( is_same_var( instr.defines, id, version) )
            {
                return &instr;
            }
        }
    }
    return nullptr;
}

std::optional<BasicInductionVar>
match_basic_induction( ir::Loop&     loop,
                       ir::PhiNode&  phi,
                       ir::Function& func)
{
    //
    // Looking only for induction variables in form:
    //
    // BB_PREV:
    //  ...
    //  i0 = INIT
    //  if ( ... ) goto BB_PREHEADER; else goto BB_NEXT
    // BB_PREHEADER:
    //  ... (empty now)
    //  goto BB_HEADER
    // BB_HEADER:
    //  i1 = PHI{BB_PREHEADER:i0, BB_EXITING:i2}
    //  ...
    //  i2 = i1 + const
    //  if ( ... ) goto BB_HEADER; else goto BB_NEXT
    //
    if ( phi.mapping.size() != 2 )
    {
        return std::nullopt;
    }

    ir::Operand init_operand{};
    ir::Operand backedge_operand{};
    bool found_backedge = false;
    for ( const auto& [bb_from, operand] : phi.mapping )
    {
        if ( bb_from == loop.preheader )
        {
            init_operand = operand;
        } else if ( bb_from == loop.exiting )
        {
            backedge_operand = operand;
            found_backedge = true;
        }
    }
    // If PHI is for something else
    if ( !found_backedge )
    {
        return std::nullopt;
    }

    // If definition of backedge operand is something strage
    // (we look only for i2 = i1 + const, later for i2 = i1 + loop_invariant)
    if ( backedge_operand.type != ir::Operand::VARIABLE )
    {
        return std::nullopt;
    }

    ir::Instruction *update = find_definition( func, backedge_operand.id, backedge_operand.value);
    if ( update == nullptr )
    {
        return std::nullopt;
    }

    // Looking only for BACKEDGE_OPERAND = PHI_RESULT + CONST
    if ( update->opcode != ir::Opcode::ADD &&
         update->opcode != ir::Opcode::SUB )
    {
        return std::nullopt;
    }

    bool matched = false;
    ir::ImmType step = 0;

    if ( is_same_var( update->operands[0], phi.var.id, phi.var.value) &&
         update->operands[1].type == ir::Operand::IMMEDIATE )
    {
        matched = true;
        if ( update->opcode == ir::Opcode::ADD )
        {
            step = update->operands[1].value;
        } else
        {
            step = -update->operands[1].value;
        }
    }
    if ( is_same_var( update->operands[1], phi.var.id, phi.var.value) &&
         update->operands[0].type == ir::Operand::IMMEDIATE )
    {
        matched = true;
        if ( update->opcode == ir::Opcode::ADD )
        {
            step = update->operands[0].value;
        } else
        {
            step = -update->operands[0].value;
        }
    }

    if ( !matched )
    {
        return std::nullopt;
    }

    return BasicInductionVar{ &phi,
                              &loop,
                              init_operand,
                              ir::Operand{ ir::Operand::IMMEDIATE, step}};
}

} // ! anonymous namespace

std::vector<BasicInductionVar>
GetInductiveVariables( ir::Program& program)
{
    std::vector<BasicInductionVar> inductions{};

    for ( ir::Function& func : program.Functions() )
    {
        for ( ir::Loop& loop : func.GetLoops() )
        {
            LOGGER(IND_VAR) << "LOOP";
            ir::BasicBlock& header = func.GetBasicBlock( loop.header);
            for ( ir::PhiNode& phi : header.phi_nodes )
            {
                std::optional<BasicInductionVar> induction = match_basic_induction( loop, phi, func);
                if ( induction.has_value() )
                {
                    LOGGER(IND_VAR) << "Found induction variable: init = "
                                    << induction.value().init.ToStr()
                                    << "step = " << induction.value().step.ToStr();
                    inductions.emplace_back( induction.value());
                }
            }
        }
    }

    return inductions;
}

} // ! namespace iv
} // ! namespace dumb
