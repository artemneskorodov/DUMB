#include <optional>

#include "iv_search.hh"
#include "logger.hh"

namespace dumb
{
namespace iv
{

namespace
{

bool
is_same_var( const ir::Operand& operand,
             const ir::SSAKey&  var)
{
    return ( (operand.type  == ir::Operand::VARIABLE) &&
             (operand.id    == var.id) &&
             (operand.value == var.version) );
}

ir::Instruction*
find_definition( ir::Function&     func,
                 const ir::SSAKey& var)
{
    for ( ir::BasicBlock& block : func.BasicBlocks() )
    {
        for ( ir::Instruction& instr : block.instructions )
        {
            if ( is_same_var( instr.defines, var) )
            {
                return &instr;
            }
        }
    }
    return nullptr;
}

std::pair<bool, ir::ImmType>
is_add_var_const( const ir::SSAKey&      var,
                  const ir::Instruction& instr)
{
    ir::ImmType sign;
    if ( instr.opcode == ir::Opcode::ADD )
    {
        sign = 1;
    } else if ( instr.opcode == ir::Opcode::SUB )
    {
        sign = -1;
    } else
    {
        return { false, 0};
    }

    if ( is_same_var( instr.operands[0], var) &&
         instr.operands[1].type == ir::Operand::IMMEDIATE )
    {
        return { true, sign * instr.operands[1].value};
    }
    if ( is_same_var( instr.operands[1], var) &&
         instr.operands[0].type == ir::Operand::IMMEDIATE )
    {
        return { true, sign * instr.operands[0].value};
    }
    return { false, 0};
}

std::optional<ir::Operand>
find_backedge_operand( ir::PhiNode& phi,
                       loops::Loop& loop)
{
    auto it = phi.mapping.find( loop.latch);
    if ( it != phi.mapping.end() )
    {
        return it->second;
    } else
    {
        return std::nullopt;
    }
}

std::optional<ir::Operand>
find_init_operand( ir::PhiNode& phi,
                   loops::Loop& loop)
{
    auto it = phi.mapping.find( loop.preheader);
    if ( it != phi.mapping.end() )
    {
        return it->second;
    } else
    {
        return std::nullopt;
    }
}

std::optional<BasicInductionVar>
match_basic_induction( loops::Loop&  loop,
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

    std::optional<ir::Operand> init_operand     = find_init_operand( phi, loop);
    std::optional<ir::Operand> backedge_operand = find_backedge_operand( phi, loop);
    if ( !init_operand.has_value() ||
         !backedge_operand.has_value() )
    {
        return std::nullopt;
    }

    // If definition of backedge operand is something strage
    // (we look only for i2 = i1 + const, later for i2 = i1 + loop_invariant)
    if ( backedge_operand.value().type != ir::Operand::VARIABLE )
    {
        return std::nullopt;
    }

    ir::Instruction *update = find_definition( func, backedge_operand.value());
    if ( update == nullptr )
    {
        return std::nullopt;
    }

    // Looking only for BACKEDGE_OPERAND = PHI_RESULT + CONST
    std::pair<bool, ir::ImmType> proccess_add_sub = is_add_var_const( phi.var, *update);
    if ( !proccess_add_sub.first )
    {
        return std::nullopt;
    }
    ir::ImmType step = proccess_add_sub.second;

    return BasicInductionVar{ &phi,
                              &loop,
                              init_operand.value(),
                              ir::Operand{ ir::Operand::IMMEDIATE, step}};
}

struct MatchMul
{
    BasicInductionVar *base;
    ir::ImmType        mul;

};

BasicInductionVar *
find_basic_induction( const ir::SSAKey& var,
                      BasicIndVarList&  basic_inductions)
{
    for ( BasicInductionVar& induction : basic_inductions )
    {
        if ( induction.phi_node->var == var )
        {
            return &induction;
        }
    }
    return nullptr;
}

std::optional<MatchMul>
match_mul_basic_induction( ir::Instruction& instr,
                           BasicIndVarList& basic_inductions)
{
    if ( instr.opcode != ir::Opcode::MUL )
    {
        return std::nullopt;
    }

    BasicInductionVar *basic_induction = nullptr;
    ir::ImmType        mul = 0;

    if ( instr.operands[0].type == ir::Operand::VARIABLE &&
         instr.operands[1].type == ir::Operand::IMMEDIATE )
    {
        basic_induction = find_basic_induction( instr.operands[0], basic_inductions);
        mul             = instr.operands[1].value;
    } else if ( instr.operands[0].type == ir::Operand::IMMEDIATE &&
                instr.operands[1].type == ir::Operand::VARIABLE )
    {
        basic_induction = find_basic_induction( instr.operands[1], basic_inductions);
        mul             = instr.operands[0].value;
    }
    if ( basic_induction == nullptr )
    {
        return std::nullopt;
    }

    return MatchMul{ basic_induction, mul};
}

std::optional<DerivedInductionVar>
match_derived_induction( ir::Instruction& instr,
                         ir::Function&    func,
                         BasicIndVarList& basic_inductions)
{
    // Looking if instr is add instruction for with constant or some induction variable and
    // if it uses result of multiplication with induction variable or constant

    // In case x = CONST * i
    std::optional<MatchMul> match_mul = match_mul_basic_induction( instr, basic_inductions);
    if ( match_mul.has_value() )
    {
        return DerivedInductionVar{ match_mul.value().base,
                                    nullptr,
                                    &instr,
                                    match_mul.value().mul,
                                    0};
    }

    // In case x = tmp + CONST
    ir::ImmType sign;
    if ( instr.opcode == ir::Opcode::ADD )
    {
        sign = 1;
    } else if ( instr.opcode == ir::Opcode::SUB )
    {
        sign = -1;
    } else
    {
        return std::nullopt;
    }

    ir::SSAKey  tmp;
    ir::ImmType offset;
    if ( instr.operands[0].type == ir::Operand::VARIABLE &&
         instr.operands[1].type == ir::Operand::IMMEDIATE )
    {
        tmp    = instr.operands[0];
        offset = instr.operands[1].value;
    } else if ( instr.operands[0].type == ir::Operand::IMMEDIATE &&
                instr.operands[1].type == ir::Operand::VARIABLE )
    {
        tmp    = instr.operands[1];
        offset = instr.operands[0].value;
    } else
    {
        return std::nullopt;
    }
    offset *= sign;

    ir::Instruction *tmp_def = find_definition( func, tmp);
    if ( tmp_def == nullptr )
    {
        return std::nullopt;
    }

    std::optional<MatchMul> nested_mul = match_mul_basic_induction( *tmp_def, basic_inductions);
    if ( !nested_mul.has_value() )
    {
        return std::nullopt;
    }
    return DerivedInductionVar{ nested_mul.value().base,
                                &instr,
                                tmp_def,
                                nested_mul.value().mul,
                                offset};
}

} // ! anonymous namespace

BasicIndVarList
GetInductiveVariables( ir::Function&           func,
                       const loops::LoopsInfo& loops)
{
    BasicIndVarList inductions{};

    for ( const std::unique_ptr<loops::Loop>& loop : loops.Loops() )
    {
        ir::BasicBlock& header = func.GetBasicBlock( loop->header);
        for ( ir::PhiNode& phi : header.phi_nodes )
        {
            std::optional<BasicInductionVar> induction = match_basic_induction( *loop,
                                                                                phi,
                                                                                func);
            if ( induction.has_value() )
            {
                inductions.emplace_back( induction.value());
            }
        }
    }

    return inductions;
}

DerivedIndVarList
GetDerivedInductiveVariables( ir::Function&           func,
                              const loops::LoopsInfo& loops,
                              BasicIndVarList&        basic_inductions)
{
    DerivedIndVarList inductions{};

    for ( const std::unique_ptr<loops::Loop>& loop : loops.Loops() )
    {
        for ( ir::BasicBlockID block_id : loop->blocks )
        {
            if ( !loop->IsBlockDirectlyInLoop( block_id) )
            {
                continue;
            }
            ir::BasicBlock& block = func.GetBasicBlock( block_id);
            for ( ir::Instruction& instr : block.instructions )
            {
                std::optional<DerivedInductionVar> induction = match_derived_induction( instr,
                                                                                        func,
                                                                                        basic_inductions);
                if ( induction.has_value() )
                {
                    inductions.emplace_back( induction.value());
                }
            }
        }
    }

    return inductions;
}

} // ! namespace iv
} // ! namespace dumb
