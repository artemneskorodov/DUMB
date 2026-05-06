#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>

#include "ir.hh"

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

using ValueMap = std::unordered_map<ir::SSAKey, LatticeValue, ir::SSAKeyHash>;
using ExecSet  = std::unordered_set<ir::BasicBlockID>;

LatticeValue
eval_operand( const ir::Operand& op,
              const ValueMap& values)
{
    if ( op.type == ir::Operand::IMMEDIATE )
    {
        return LatticeValue::Constant( op.value);
    }

    if ( op.type == ir::Operand::VARIABLE )
    {
        auto it = values.find( { op.id, op.value});
        if (it == values.end())
        {
            return LatticeValue::Overdefined();
        }
        return it->second;
    }

    return LatticeValue::Overdefined();
}

LatticeValue
eval_instr( const ir::Instruction& instr,
            const ValueMap& values)
{
    if ( instr.opcode == ir::Opcode::MOV )
    {
        return eval_operand( instr.operands[0], values);
    }

    if ( (instr.opcode == ir::Opcode::ADD) ||
         (instr.opcode == ir::Opcode::SUB) ||
         (instr.opcode == ir::Opcode::MUL) ||
         (instr.opcode == ir::Opcode::DIV) )
    {
        auto v1 = eval_operand( instr.operands[0], values);
        auto v2 = eval_operand( instr.operands[1], values);

        if ( (v1.kind == LatticeKind::CONSTANT) &&
             (v2.kind == LatticeKind::CONSTANT) )
        {
            switch ( instr.opcode )
            {
                case ir::Opcode::ADD: return LatticeValue::Constant( v1.constant + v2.constant);
                case ir::Opcode::SUB: return LatticeValue::Constant( v1.constant - v2.constant);
                case ir::Opcode::MUL: return LatticeValue::Constant( v1.constant * v2.constant);
                case ir::Opcode::DIV: return LatticeValue::Constant( v1.constant / v2.constant);
                default:              return LatticeValue::Overdefined();
            }
        }

        if ( (v1.kind == LatticeKind::OVERDEFINED) ||
             (v2.kind == LatticeKind::OVERDEFINED) )
        {
            return LatticeValue::Overdefined();
        }

        return LatticeValue::Undefined();
    }

    return LatticeValue::Overdefined();
}

} // anonymous namespace

void
SparseConditionalConstantPropagation( ir::Program& program)
{
    for ( ir::Function& func : program.Functions() )
    {
        ValueMap values{};    // Lattice map of all SSA Keys
        ExecSet executable{}; // Reachable blocks set

        std::queue<ir::BasicBlockID> control_flow_worklist{}; // Queue of control flow
        std::queue<ir::BasicBlockID> ssa_worklist{};          // Queue of SSA

        ir::BasicBlockID entry = func.Entry();

        executable.insert( entry);
        control_flow_worklist.push( entry);

        while ( (!control_flow_worklist.empty()) ||
                (!ssa_worklist.empty()) )
        {
            while ( !control_flow_worklist.empty() )
            {
                ir::BasicBlockID bb_id = control_flow_worklist.front();
                control_flow_worklist.pop();
                ssa_worklist.push( bb_id);
            }

            while ( !ssa_worklist.empty() )
            {
                ir::BasicBlockID bb_id = ssa_worklist.front();
                ssa_worklist.pop();
                const ir::BasicBlock& bb = func.GetBasicBlock( bb_id);

                for ( const ir::PhiNode& phi : bb.phi_nodes )
                {
                    LatticeValue val = LatticeValue::Undefined();

                    for ( auto& [pred_id, operand] : phi.mapping )
                    {
                        if ( executable.count( pred_id) == 0 )
                        {
                            continue;
                        }

                        val = LatticeValue::Merge( val, eval_operand( operand, values));
                    }

                    ir::SSAKey key{ phi.var.id, phi.var.value};

                    if ( values[key] != val )
                    {
                        values[key] = val;
                        ssa_worklist.push( bb_id);
                    }
                }

                for ( const ir::Instruction& instr : bb.instructions )
                {
                    if ( instr.defines.type != ir::Operand::VARIABLE )
                    {
                        continue;
                    }

                    ir::SSAKey key{ instr.defines.id, instr.defines.value};

                    LatticeValue val = eval_instr( instr, values);

                    if ( values[key] != val )
                    {
                        values[key] = val;
                        ssa_worklist.push( bb_id);
                    }
                }

                const ir::BasicBlockTerminator& term = bb.terminator;

                if ( term.type == ir::CmpType::ALWAYS_TRUE )
                {
                    if (!executable.count( term.true_dest))
                    {
                        executable.insert( term.true_dest);
                        control_flow_worklist.push( term.true_dest);
                    }
                } else if ( term.type != ir::CmpType::INVALID )
                {
                    LatticeValue left  = eval_operand( term.left, values);
                    LatticeValue right = eval_operand( term.right, values);

                    bool is_const = false;
                    bool result   = false;

                    if ( (left.kind == LatticeKind::CONSTANT) &&
                         (right.kind == LatticeKind::CONSTANT) )
                    {
                        is_const = true;
                        switch ( term.type )
                        {
                            case ir::CmpType::LESS:   result = (left.constant <  right.constant); break;
                            case ir::CmpType::EQUAL:  result = (left.constant == right.constant); break;
                            case ir::CmpType::BIGGER: result = (left.constant >  right.constant); break;
                            default: throw std::runtime_error{ "Unexpected condition type"};
                        }
                    }

                    if ( is_const )
                    {
                        ir::BasicBlockID target_id = result ? term.true_dest : term.false_dest;

                        if ( !executable.count( target_id) )
                        {
                            executable.insert( target_id);
                            control_flow_worklist.push(target_id);
                        }
                    }
                    else
                    {
                        if ( !executable.count( term.true_dest) )
                        {
                            executable.insert( term.true_dest);
                            control_flow_worklist.push( term.true_dest);
                        }
                        if ( !executable.count( term.false_dest) )
                        {
                            executable.insert( term.false_dest);
                            control_flow_worklist.push( term.false_dest);
                        }
                    }
                }
            }
        }

        for ( ir::BasicBlock& bb : func.BasicBlocks() )
        {
            for ( ir::Instruction& instr : bb.instructions )
            {
                for ( ir::Operand& op : instr.operands )
                {
                    if (op.type == ir::Operand::VARIABLE)
                    {
                        auto it = values.find( { op.id, op.value});
                        if ( (it != values.end()) &&
                             (it->second.kind == LatticeKind::CONSTANT) )
                        {
                            op.type  = ir::Operand::IMMEDIATE;
                            op.value = it->second.constant;
                        }
                    }
                }
            }

            for ( ir::PhiNode& phi : bb.phi_nodes )
            {
                for ( auto& [bb_id, operand] : phi.mapping )
                {
                    if ( operand.type != ir::Operand::VARIABLE )
                    {
                        continue;
                    }
                    auto it = values.find( { operand.id, operand.value});
                    if ( (it != values.end()) &&
                         (it->second.kind == LatticeKind::CONSTANT) )
                    {
                        operand.type  = ir::Operand::IMMEDIATE;
                        operand.value = it->second.constant;
                    }
                }
            }

            ir::BasicBlockTerminator& term = bb.terminator;
            if ( (term.type != ir::CmpType::ALWAYS_TRUE) &&
                 (term.type != ir::CmpType::INVALID) )
            {
                if ( term.left.type == ir::Operand::VARIABLE )
                {
                    auto it = values.find( { term.left.id, term.left.value});
                    if ( it != values.end() && it->second.kind == LatticeKind::CONSTANT )
                    {
                        term.left.type  = ir::Operand::IMMEDIATE;
                        term.left.value = it->second.constant;
                    }
                }
                if ( term.right.type == ir::Operand::VARIABLE )
                {
                    auto it = values.find( { term.right.id, term.right.value});
                    if ( it != values.end() && it->second.kind == LatticeKind::CONSTANT )
                    {
                        term.right.type  = ir::Operand::IMMEDIATE;
                        term.right.value = it->second.constant;
                    }
                }
                if ( (term.left.type  == ir::Operand::IMMEDIATE) &&
                     (term.right.type == ir::Operand::IMMEDIATE) )
                {
                    int left  = term.left.value;
                    int right = term.right.value;
                    bool result;
                    switch ( term.type )
                    {
                        case ir::CmpType::LESS:   result = (left <  right); break;
                        case ir::CmpType::EQUAL:  result = (left == right); break;
                        case ir::CmpType::BIGGER: result = (left >  right); break;
                        default: throw std::runtime_error{ "Unexpected condition type"};
                    }

                    term.type = ir::CmpType::ALWAYS_TRUE;
                    if ( result )
                    {
                        ir::BasicBlock& false_bb = func.GetBasicBlock( term.false_dest);
                        for ( ir::PhiNode& phi : false_bb.phi_nodes )
                        {
                            if ( phi.mapping.count( bb.id) != 0 )
                            {
                                phi.mapping.erase( bb.id);
                                bb.phi_acceptors.remove( term.false_dest);
                            }
                        }
                        false_bb.predecessors.remove( bb.id);
                        term.true_dest  = term.true_dest;
                        term.false_dest = 0;
                    } else
                    {
                        ir::BasicBlock& true_bb = func.GetBasicBlock( term.true_dest);
                        for ( ir::PhiNode& phi : true_bb.phi_nodes )
                        {
                            if ( phi.mapping.count( bb.id) != 0 )
                            {
                                phi.mapping.erase( bb.id);
                                bb.phi_acceptors.remove( term.true_dest);
                            }
                        }
                        true_bb.predecessors.remove( bb.id);
                        term.true_dest  = term.false_dest;
                        term.false_dest = 0;
                    }
                }
            }
        }
    }
}

} // namespace sccp
} // namespace dumb
