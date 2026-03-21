#include <unordered_map>
#include <iostream>

#include "ir.hh"
#include "backend.hh"
#include "lir.hh"

namespace dumb
{

namespace
{

class OperandEmitter : public ir::OperandVisitor
{
public:
    OperandEmitter( const std::unordered_map<ir::VarID, int>& rbp_offsets)
     :  rbp_offsets_{ rbp_offsets}
    {
    }

    void
    Visit( ir::VarOperand& node) override
    {
        int rbp_offset = rbp_offsets_.find( node.id)->second;
        result_ = lir::RegMem{ lir::Register::RBP, rbp_offset};
    }

    void
    Visit( ir::ImmOperand& node) override
    {
        result_ = lir::Immediate{ node.value};
    }

    void
    Visit( ir::GVarOperand& node) override
    {
        result_ = lir::Memory{ "GLOBAL_" + std::to_string( node.id)};
    }

    lir::Operand
    GetOperand( ir::Operand *operand)
    {
        operand->Accept( *this);
        return result_;
    }
private:
    lir::Operand result_{ lir::Register::RAX};
    const std::unordered_map<ir::VarID, int>& rbp_offsets_;

};

class InstructionEmitter : public ir::InstructionVisitor
{
public:
    InstructionEmitter( const std::unordered_map<ir::VarID, int>& rbp_offsets,
                        lir::Program& lir,
                        int variables_size)
     :  op_emitter_{ rbp_offsets},
        lir_{ &lir},
        variables_size_{ variables_size}
    {
    }

    void
    Visit( ir::BinaryOpInstr& node) override
    {
        lir_->AddMov( lir::Register::RAX, op_emitter_.GetOperand( node.first.get()));
        lir_->AddMov( lir::Register::RBX, op_emitter_.GetOperand( node.second.get()));

        lir::MathType type;

        switch ( node.op )
        {
            case ir::BinaryOpType::ADD: type = lir::MathType::ADD; break;
            case ir::BinaryOpType::SUB: type = lir::MathType::ADD; break;
            case ir::BinaryOpType::MUL: type = lir::MathType::ADD; break;
            case ir::BinaryOpType::DIV: type = lir::MathType::ADD; break;
        }

        lir_->AddMath( type, lir::Register::RAX, lir::Register::RBX);
        lir_->AddMov( op_emitter_.GetOperand( node.dest.get()), lir::Register::RAX);
    }

    void
    Visit( ir::UnaryOpInstr& node) override
    {
        if ( node.op == ir::UnaryOpType::MOV )
        {
            lir_->AddMov( lir::Register::RAX, op_emitter_.GetOperand( node.operand.get()));
            lir_->AddMov( op_emitter_.GetOperand( node.dest.get()), lir::Register::RAX);
        } else if ( node.op == ir::UnaryOpType::RET )
        {
            lir_->AddMov( lir::Register::RAX, op_emitter_.GetOperand( node.operand.get()));
            lir_->AddMath( lir::MathType::SUB, lir::Register::RSP, lir::Immediate{ variables_size_});
            lir_->AddRet();
        }
    }

    void
    Visit( ir::FunctionCallInstr& node) override
    {
        for ( auto& it : node.params )
        {
            lir_->AddPush( op_emitter_.GetOperand( it.get()));
        }
        lir_->AddMath( lir::MathType::ADD, lir::Register::RBP, lir::Immediate{ 1});
        lir_->AddPush( lir::Register::RBP);
        lir_->AddCall( "FUNC_" + std::to_string( node.func));
        lir_->AddPop( lir::Register::RBP);
        lir_->AddMov( op_emitter_.GetOperand( node.dest.get()), lir::Register::RAX);
    }

    void
    Visit( ir::CmpAndJmpInstr& node) override
    {
        if ( node.type == ir::CmpType::ALWAYS_TRUE )
        {
            lir_->AddJmp( lir::JmpType::JMP, ".LOC_" + std::to_string( node.true_dest));
            return ;
        }

        node.left->Accept( op_emitter_);
        lir_->AddMov( lir::Register::RAX, op_emitter_.GetOperand( node.left.get()));
        lir_->AddMov( lir::Register::RBX, op_emitter_.GetOperand( node.right.get()));
        lir_->AddMath( lir::MathType::CMP, lir::Register::RAX, lir::Register::RBX);

        std::string true_label = ".LOC_" + std::to_string( node.true_dest);
        std::string false_label = ".LOC_" + std::to_string( node.false_dest);

        if ( node.type == ir::CmpType::LESS )
        {
            lir_->AddJmp( lir::JmpType::JL, true_label);
            lir_->AddJmp( lir::JmpType::JG, false_label);
        } else if ( node.type == ir::CmpType::EQUAL )
        {
            lir_->AddJmp( lir::JmpType::JE,  true_label);
            lir_->AddJmp( lir::JmpType::JNE, false_label);
        } else if ( node.type == ir::CmpType::BIGGER )
        {
            lir_->AddJmp( lir::JmpType::JG, true_label);
            lir_->AddJmp( lir::JmpType::JL, false_label);
        } else
        {
            throw std::runtime_error{ "Unexpected comparison type"};
        }
    }

    void
    Visit( ir::InputInstr& node) override
    {
        lir_->AddString( node.string);
        std::string string_label = "GLOBAL_STRING_" + std::to_string( lir_->StringsNum());
        lir_->AddMov( lir::Register::RSI, lir::StringImm{ string_label});
        lir_->AddMov( lir::Register::RDX, lir::Immediate{ static_cast<int>( node.string.length())});
        lir_->AddPush( lir::Register::RBP);
        lir_->AddCall( "__std_input");
        lir_->AddPop( lir::Register::RBP);
        lir_->AddMov( op_emitter_.GetOperand( node.dest.get()), lir::Register::RAX);
    }

    void
    Visit( ir::OutputInstr& node) override
    {
        lir_->AddString( node.string);
        std::string string_label = "GLOBAL_STRING_" + std::to_string( lir_->StringsNum());
        lir_->AddMov( lir::Register::RSI, lir::StringImm{ string_label});
        lir_->AddMov( lir::Register::RDX, lir::Immediate{ static_cast<int>( node.string.length())});
        lir_->AddMov( lir::Register::RCX, op_emitter_.GetOperand( node.expression.get()));
        lir_->AddPush( lir::Register::RBP);
        lir_->AddCall( "__std_output");
        lir_->AddPop( lir::Register::RBP);
    }

    void
    EmitLIR( lir::Program& lir,
             ir::Instruction *instr)
    {
        lir_ = &lir;
        instr->Accept( *this);
    }

private:
    OperandEmitter op_emitter_;
    lir::Program *lir_;
    int variables_size_;

};

void
EmitBasicBlock( lir::Program& lir,
                ir::BasicBlock *basic_block,
                const std::unordered_map<ir::VarID, int>& rbp_offsets,
                int variables_size)
{
    InstructionEmitter emitter{ rbp_offsets, lir, variables_size};

    lir.AddLabel( ".LOC_" + std::to_string( basic_block->id));

    for ( auto& instr : basic_block->instructions )
    {
        emitter.EmitLIR( lir, instr.get());
    }
}

void
EmitFunction( lir::Program& lir,
              ir::Function *function,
              bool need_label = true)
{
    std::unordered_map<ir::VarID, int> rbp_offsets;

    for ( size_t i = 0; i != function->params.size(); ++i )
    {
        rbp_offsets[function->params[i]] = 8 * (static_cast<int>( i) + 1);
    }
    for ( size_t i = 0; i != function->variables.size(); ++i )
    {
        rbp_offsets[function->variables[i]] = - 8 * (static_cast<int>( i) + 1);
    }

    if ( need_label )
    {
        lir.AddLabel( "FUNC_" + std::to_string( function->id));
    }

    lir.AddMov( lir::Register::RBP, lir::Register::RSP);
    lir.AddMath( lir::MathType::SUB, lir::Register::RBP, lir::Immediate{ 8});
    int variables_size = static_cast<int>( function->variables.size() * 8);
    lir.AddMath( lir::MathType::ADD, lir::Register::RSP, lir::Immediate{ variables_size});

    for ( auto& block : function->basic_blocks )
    {
        EmitBasicBlock( lir, block.get(), rbp_offsets, variables_size);
    }
}

lir::Program
EmitLowLevelIR( ir::Program *program)
{
    lir::Program lir;

    lir.AddLabel( "_start");

    EmitFunction( lir, program->preamble.get(), false);
    lir.AddCall( "FUNC_0"); // TODO FIXME, find main in nametable, add nametable to common

    lir.AddMov( lir::Register::RAX, lir::Immediate{ 60});
    lir.AddMath( lir::MathType::XOR, lir::Register::RDI, lir::Register::RDI);
    lir.AddSyscall();

    for ( auto& func : program->functions )
    {
        EmitFunction( lir, func.get());
    }

    for ( auto& global : program->globals )
    {
        lir.AddGlobal( "GLOBAL_" + std::to_string( global), 0);
    }

    return lir;
}

} // ! anonymous namespace

std::string
RunBackend( ir::Program *program)
{
    lir::Program lir = EmitLowLevelIR( program);
    return lir.ToStr();
}

} // ! namespace dumb
