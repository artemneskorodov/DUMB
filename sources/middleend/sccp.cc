#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>

#include "ir.hh"
#include "sccp.hh"
#include "ssa_framework.hh"

namespace dumb
{
namespace sccp
{

namespace
{

enum class LatticeKind
{
    UNDEFINED,
    CONSTANT,
    OVERDEFINED,
};

struct LatticeValue
{
    static LatticeValue Undefined   ()               { return { LatticeKind::UNDEFINED,   0}; }
    static LatticeValue Constant    ( ir::ImmType v) { return { LatticeKind::CONSTANT,    v}; }
    static LatticeValue Overdefined ()               { return { LatticeKind::OVERDEFINED, 0}; }

    static LatticeValue
    Merge( const LatticeValue& first,
           const LatticeValue& second)
    {
        if ( first.kind == LatticeKind::UNDEFINED )
        {
            return second;
        }
        if ( second.kind == LatticeKind::UNDEFINED )
        {
            return first;
        }
        if ( (first.kind  == LatticeKind::OVERDEFINED) ||
             (second.kind == LatticeKind::OVERDEFINED) )
        {
            return LatticeValue::Overdefined();
        }

        if ( first.constant == second.constant )
        {
            return first;
        }

        return LatticeValue::Overdefined();
    }

    bool
    operator==( const LatticeValue& o) const
    {
        return (kind == o.kind) &&
               ( (kind != LatticeKind::CONSTANT) ||
                 (constant == o.constant) );
    }

    bool
    operator!=( const LatticeValue& o) const
    {
        return !(*this == o);
    }

    LatticeKind kind;
    int constant;
};

bool
EvaluateCmp( ir::ImmType left,
             ir::ImmType right,
             ir::CmpType type)
{
    switch ( type )
    {
        case ir::CmpType::LESS:   return (left <  right);
        case ir::CmpType::EQUAL:  return (left == right);
        case ir::CmpType::BIGGER: return (left >  right);
        default: throw std::runtime_error{ "Unexpected condition type"};
    }
}

///
/// @brief Type which is returned by evaluator. Maps variables to their values if they are defined.
///
using ValueMap = std::unordered_map<ir::SSAKey, LatticeValue, ir::SSAKeyHash>;

///
/// @brief Evaluator of SCCP.
///
/// while cfg_worklist or ssa_worklist not empty:
///     for each basic_block in cfg_worklist:
///         evaluate phi_nodes
///         evaluate instructions
///         evaluate terminator
///         if something changed( variable became defined, or undefined):
///             push variable to ssa_worklist
///
///     for each variable in ssa_worklist:
///         evaluate all users of this variable
///
class SccpEvaluator
{
public:
    SccpEvaluator( const ir::Function& func)
     :  func_{ func}
    {
    }

    ValueMap
    Evaluate()
    {
        ir::BasicBlockID entry = func_.Entry();

        executable_blocks_.insert( entry);
        cfg_worklist_.push( entry);

        while ( (!cfg_worklist_.empty()) ||
                (!ssa_worklist_.empty()) )
        {
            while ( !cfg_worklist_.empty() )
            {
                ir::BasicBlockID bb_id = cfg_worklist_.front();
                cfg_worklist_.pop();
                evaluate_basic_block( func_.GetBasicBlock( bb_id));
            }
            while ( !ssa_worklist_.empty() )
            {
                ir::SSAKey ssa_key = ssa_worklist_.front();
                ssa_worklist_.pop();
                for ( const ir::PhiNode *phi : ssa::GetPhiUsers( func_, ssa_key) )
                {
                    evaluate_phi( *phi);
                }
                for ( const ir::Instruction *instr : ssa::GetInstrUsers( func_, ssa_key))
                {
                    evaluate_instr( *instr);
                }
            }
        }

        return values_;
    }

private:
    void
    evaluate_instr( const ir::Instruction& instr)
    {
        if ( instr.defines.type != ir::Operand::VARIABLE )
        {
            // Evaluate only evaluatable instructions (defines can be VARIABLE, GLOBAL or EMPTY)
            return ;
        }

        LatticeValue value = LatticeValue::Overdefined();

        if ( instr.opcode == ir::Opcode::MOV )
        {
            value = eval_operand( instr.operands[0]);
        } else if ( (instr.opcode == ir::Opcode::ADD) ||
                    (instr.opcode == ir::Opcode::SUB) ||
                    (instr.opcode == ir::Opcode::MUL) ||
                    (instr.opcode == ir::Opcode::DIV) )
        {
            LatticeValue first  = eval_operand( instr.operands[0]);
            LatticeValue second = eval_operand( instr.operands[1]);

            if ( (first.kind  == LatticeKind::CONSTANT) &&
                 (second.kind == LatticeKind::CONSTANT) )
            {
                ir::ImmType result;
                switch ( instr.opcode )
                {
                    case ir::Opcode::ADD: result = first.constant + second.constant; break;
                    case ir::Opcode::SUB: result = first.constant - second.constant; break;
                    case ir::Opcode::MUL: result = first.constant * second.constant; break;
                    case ir::Opcode::DIV: result = first.constant / second.constant; break;
                    default: throw std::runtime_error{ "Unreachable"};
                }
                value = LatticeValue::Constant( result);
            }

            if ( (first.kind  == LatticeKind::OVERDEFINED) ||
                 (second.kind == LatticeKind::OVERDEFINED) )
            {
                value = LatticeValue::Overdefined();
            }

            value = LatticeValue::Undefined();
        }

        update_value( instr.defines, value);
    }

    void
    evaluate_phi( const ir::PhiNode& phi)
    {
        LatticeValue value = LatticeValue::Undefined();

        for ( auto& [pred_id, operand] : phi.mapping )
        {
            if ( !is_executable( pred_id) )
            {
                continue;
            }
            value = LatticeValue::Merge( value, eval_operand( operand));
        }
        update_value( phi.var, value);
    }

    void
    evaluate_basic_block( const ir::BasicBlock& block)
    {
    // Trying to evaluate Phi nodes
    for ( const ir::PhiNode& phi : block.phi_nodes )
    {
        evaluate_phi( phi);
    }

    // Trying to evaluate instructions
    for ( const ir::Instruction& instr : block.instructions )
    {
        evaluate_instr( instr);
    }

    // Trying to evaluate terminator
    const ir::BasicBlockTerminator& term = block.terminator;
    if ( term.type == ir::CmpType::ALWAYS_TRUE )
    {
        set_executable( term.true_dest);
        cfg_worklist_.push( term.true_dest);
    } else if ( term.type != ir::CmpType::INVALID )
    {
        LatticeValue left  = eval_operand( term.left);
        LatticeValue right = eval_operand( term.right);

        if ( (left.kind  == LatticeKind::CONSTANT) &&
             (right.kind == LatticeKind::CONSTANT) )
        {
            bool cmp_result = EvaluateCmp( left.constant, right.constant, term.type);
            ir::BasicBlockID target = cmp_result ? term.true_dest : term.false_dest;
            set_executable( target);
            cfg_worklist_.push( target);
        } else
        {
            set_executable( term.true_dest);
            set_executable( term.false_dest);
            cfg_worklist_.push( term.true_dest);
            cfg_worklist_.push( term.false_dest);
        }
    }
}

private:
    LatticeValue
    eval_operand( const ir::Operand& op)
    {
        if ( op.type == ir::Operand::IMMEDIATE )
        {
            return LatticeValue::Constant( op.value);
        }

        if ( op.type == ir::Operand::VARIABLE )
        {
            auto it = values_.find( { op.id, op.value});
            if (it == values_.end())
            {
                return LatticeValue::Overdefined();
            }
            return it->second;
        }

        return LatticeValue::Overdefined();
    }

    bool
    is_executable( ir::BasicBlockID id) const
    {
        return (executable_blocks_.count( id) != 0);
    }

    void
    set_executable( ir::BasicBlockID id)
    {
        if ( !is_executable( id) )
        {
            executable_blocks_.insert( id);
        }
    }

    void
    update_value( const ir::SSAKey&   key,
                  const LatticeValue& value)
    {
        if ( values_[key] != value )
        {
            values_[key] = value;
            ssa_worklist_.push( key);
        }
    }

private:
    const ir::Function& func_;

    ValueMap                             values_{};
    std::unordered_set<ir::BasicBlockID> executable_blocks_{};

    std::queue<ir::BasicBlockID>         cfg_worklist_{}; // Worklist of basic blocks
    std::queue<ir::SSAKey>               ssa_worklist_{}; // Worklist of changed variables

};

void
remove_predecessor( ir::BasicBlock& from,
                    ir::BasicBlock& to_remove)
{
    for ( ir::PhiNode& phi : from.phi_nodes )
    {
        if ( phi.mapping.count( to_remove.id) != 0 )
        {
            phi.mapping.erase( to_remove.id);
            to_remove.phi_acceptors.remove( from.id);
        }
    }
    from.predecessors.remove( to_remove.id);
}

} // anonymous namespace

void
SparseConditionalConstantPropagation( ir::Program& program)
{
    for ( ir::Function& func : program.Functions() )
    {
        // Evaluate function with SCCP
        SccpEvaluator evaluator{ func};
        ValueMap values = evaluator.Evaluate();

        // Update values
        for ( const auto& [ssa_key, lattice_value] : values )
        {
            if ( lattice_value.kind != LatticeKind::CONSTANT )
            {
                continue;
            }

            ir::Operand operand = ir::Operand{ ir::Operand::IMMEDIATE, lattice_value.constant};

            for ( ir::Operand *use : ssa::GetUses( func, ssa_key) )
            {
                *use = operand;
            }
        }

        // Update basic blocks
        for ( ir::BasicBlock& block : func.BasicBlocks() )
        {
            ir::BasicBlockTerminator& term = block.terminator;

            if ( (term.type == ir::CmpType::ALWAYS_TRUE) ||
                 (term.type == ir::CmpType::INVALID) )
            {
                // Cannot do anything better for this terminator
                continue;
            }

            if ( (term.left.type  != ir::Operand::IMMEDIATE) ||
                 (term.right.type != ir::Operand::IMMEDIATE) )
            {
                // Connot simplify this terminator
                continue;
            }

            bool cmp_result = EvaluateCmp( term.left.value, term.right.value, term.type);

            term.type = ir::CmpType::ALWAYS_TRUE;

            if ( cmp_result )
            {
                ir::BasicBlock& unreachable = func.GetBasicBlock( term.false_dest);
                remove_predecessor( unreachable, block);
                term.false_dest = 0;
            } else
            {
                ir::BasicBlock& unreachable = func.GetBasicBlock( term.false_dest);
                remove_predecessor( unreachable, block);
                term.true_dest = term.false_dest;
                term.false_dest = 0;
            }
        }
    }
}

} // namespace sccp
} // namespace dumb
