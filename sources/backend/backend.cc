#include <unordered_map>
#include <iostream>

#include "ir.hh"
#include "backend.hh"
#include "lir.hh"

namespace dumb
{

namespace
{

class OperandEmitter : public ir::ConstantOperandVisitor
{
public:
    explicit
    OperandEmitter( std::unordered_map<std::string, int> rbp_offsets)
     :  rbp_offsets_{ std::move( rbp_offsets)}
    {
    }

    OperandEmitter() = default;

private:
    void
    Visit( const ir::VarOperand& node) override
    {
        int rbp_offset = rbp_offsets_.find( node.name)->second;
        result_ = lir::RegMem{ lir::Register::RBP, rbp_offset};
    }

    void
    Visit( const ir::GVarOperand& node) override
    {
        result_ = lir::Memory{ node.name};
    }

    void
    Visit( const ir::ImmOperand& node) override
    {
        result_ = lir::Immediate{ node.value};
    }

public:
    lir::Operand
    GetOperand( const ir::Operand& operand)
    {
        operand.Accept( *this);
        return result_;
    }

private:
    lir::Operand                         result_      { lir::Register::RAX};
    std::unordered_map<std::string, int> rbp_offsets_ {};

};

class InstructionEmitter : public ir::ConstantInstructionVisitor
{
private:
    void
    Visit( const ir::BinaryOpInstr& node) override
    {
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, op_emitter_.GetOperand( *node.first));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, op_emitter_.GetOperand( *node.second));

        switch ( node.op )
        {
            case ir::BinaryOpType::ADD:
            {
                lir_.Add( lir::BinaryOp::ADD, lir::Register::RAX, lir::Register::RBX);
                break;
            }
            case ir::BinaryOpType::SUB:
            {
                lir_.Add( lir::BinaryOp::SUB, lir::Register::RAX, lir::Register::RBX);
                break;
            }
            case ir::BinaryOpType::MUL:
            {
                lir_.Add( lir::UnaryOp::IMUL, lir::Register::RBX);
                break;
            }
            case ir::BinaryOpType::DIV:
            {
                lir_.Add( lir::BinaryOp::XOR, lir::Register::RDX, lir::Register::RDX);
                lir_.Add( lir::NoOpInstr::CQO);
                lir_.Add( lir::UnaryOp::IDIV, lir::Register::RBX);
                break;
            }
            default:
            {
                throw std::runtime_error{ "Unexpected binary op type"};
            }
        }

        lir_.Add( lir::BinaryOp::MOV, op_emitter_.GetOperand( *node.dest), lir::Register::RAX);
    }

    void
    Visit( const ir::UnaryOpInstr& node) override
    {
        if ( node.op == ir::UnaryOpType::MOV )
        {
            lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, op_emitter_.GetOperand( *node.operand));
            lir_.Add( lir::BinaryOp::MOV, op_emitter_.GetOperand( *node.dest), lir::Register::RAX);
        } else if ( node.op == ir::UnaryOpType::RET )
        {
            lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, op_emitter_.GetOperand( *node.operand));
            lir_.Add( lir::BinaryOp::ADD, lir::Register::RSP, lir::Immediate{ variables_size_});
            lir_.Add( lir::NoOpInstr::RET);
        }
    }

    void
    Visit( const ir::FunctionCallInstr& node) override
    {
        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);

        for ( const auto& it : node.params )
        {
            lir_.Add( lir::UnaryOp::PUSH, op_emitter_.GetOperand( *it));
        }
        int params_num = static_cast<int>( node.params.size());
        lir_.AddCall( node.name);
        lir_.Add( lir::BinaryOp::ADD, lir::Register::RSP, lir::Immediate{ 8 * params_num});
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
        lir_.Add( lir::BinaryOp::MOV, op_emitter_.GetOperand( *node.dest), lir::Register::RAX);
    }

    void
    Visit( const ir::CmpAndJmpInstr& node) override
    {
        if ( node.type == ir::CmpType::ALWAYS_TRUE )
        {
            lir_.AddJmp( lir::JmpType::JMP, ".LOC_" + std::to_string( node.true_dest));
            return ;
        }

        node.left->Accept( op_emitter_);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, op_emitter_.GetOperand( *node.left));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, op_emitter_.GetOperand( *node.right));
        lir_.Add( lir::BinaryOp::CMP, lir::Register::RAX, lir::Register::RBX);

        std::string true_label = ".LOC_" + std::to_string( node.true_dest);
        std::string false_label = ".LOC_" + std::to_string( node.false_dest);

        if ( node.type == ir::CmpType::LESS )
        {
            lir_.AddJmp( lir::JmpType::JL, true_label);
            lir_.AddJmp( lir::JmpType::JGE, false_label);
        } else if ( node.type == ir::CmpType::EQUAL )
        {
            lir_.AddJmp( lir::JmpType::JE,  true_label);
            lir_.AddJmp( lir::JmpType::JNE, false_label);
        } else if ( node.type == ir::CmpType::BIGGER )
        {
            lir_.AddJmp( lir::JmpType::JG, true_label);
            lir_.AddJmp( lir::JmpType::JLE, false_label);
        } else
        {
            throw std::runtime_error{ "Unexpected comparison type"};
        }
    }

    void
    Visit( const ir::InputInstr& node) override
    {
        std::string str_label = "STR_CONST_" + std::to_string( str_constr_counter_++);
        lir_.AddStrConst( str_label, node.string);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RSI, lir::StringImm{ str_label});
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RDX, node.string.length());
        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);
        lir_.AddCall( "__std_input");
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
        lir_.Add( lir::BinaryOp::MOV, op_emitter_.GetOperand( *node.dest), lir::Register::RAX);
    }

    void
    Visit( const ir::OutputInstr& node) override
    {
        std::string str_label = "STR_CONST_" + std::to_string( str_constr_counter_++);
        lir_.AddStrConst( str_label, node.string);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RSI, lir::StringImm{ str_label});
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RDX, node.string.length());
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RCX, op_emitter_.GetOperand( *node.expression));
        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);
        lir_.AddCall( "__std_output");
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
    }

    void
    EmitBasicBlock( const ir::BasicBlock& basic_block)
    {
        lir_.AddLabel( ".LOC_" + std::to_string( basic_block.id));

        for ( const auto& instr : basic_block.instructions )
        {
            instr->Accept( *this);
        }
    }

    void
    EmitFunction( const ir::Function& function)
    {
        std::unordered_map<std::string, int> rbp_offsets;

        for ( size_t i = 0; i != function.params.size(); ++i )
        {
            rbp_offsets[function.params[i]] = 8 * (static_cast<int>( i) + 1);
        }
        for ( size_t i = 0; i != function.variables.size(); ++i )
        {
            rbp_offsets[function.variables[i]] = - 8 * (static_cast<int>( i) + 1);
        }

        op_emitter_ = OperandEmitter{ std::move (rbp_offsets)};

        lir_.AddLabel( function.name);

        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBP, lir::Register::RSP);
        // lir_.AddMath( lir::MathType::SUB, lir::Register::RBP, lir::Immediate{ 8});
        variables_size_ = static_cast<int>( function.variables.size() * 8);
        lir_.Add( lir::BinaryOp::SUB, lir::Register::RSP, lir::Immediate{ variables_size_});

        for ( const auto& block : function.basic_blocks )
        {
            EmitBasicBlock( *block);
        }
    }

public:
    lir::Program
    EmitLowLevelIR( const ir::Program& program)
    {
        // Preamble
        EmitFunction( *program.preamble);
        lir_.AddCall( "main"); // TODO check that main appears in nametable
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, lir::Immediate{ 60});
        lir_.Add( lir::BinaryOp::XOR, lir::Register::RDI, lir::Register::RDI);
        lir_.Add( lir::NoOpInstr::SYSCALL);

        // Functions
        for ( const auto& func : program.functions )
        {
            EmitFunction( *func);
        }

        // Adding globals
        for ( const std::string& global : program.globals )
        {
            lir_.AddGlobal( global, 0);
        }

        return std::move( lir_);
    }

private:
    lir::Program   lir_{};
    OperandEmitter op_emitter_{};
    int            variables_size_;
    std::size_t    str_constr_counter_{0};

};

} // ! anonymous namespace

std::string
RunBackend( const ir::Program& program)
{
    InstructionEmitter emitter{};
    lir::Program lir = emitter.EmitLowLevelIR( program);
    return lir.ToStr();
}

} // ! namespace dumb
