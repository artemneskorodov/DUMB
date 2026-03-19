#include <unordered_map>
#include <iostream>

#include "ir.hh"
#include "backend.hh"

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
        if ( rbp_offset >= 0 )
        {
            result_ = "[rbp + ";
        } else
        {
            result_ = "[rbp - ";
            rbp_offset = -rbp_offset;
        }
        result_ += std::to_string( rbp_offset) + "]";
    }

    void
    Visit( ir::ImmOperand& node) override
    {
        result_ = std::to_string( node.value);;
    }

    void
    Visit( ir::GVarOperand& node) override
    {
        result_ = "[GLOBAL_" + std::to_string( node.id) + "]";
    }

    std::string
    GetStr( ir::Operand *operand)
    {
        operand->Accept( *this);
        return result_;
    }
private:
    std::string result_;
    const std::unordered_map<ir::VarID, int>& rbp_offsets_;

};

class InstructionEmitter : public ir::InstructionVisitor
{
public:
    InstructionEmitter( const std::unordered_map<ir::VarID, int>& rbp_offsets)
     :  op_emitter_{ rbp_offsets}
    {
    }

    void
    Visit( ir::BinaryOpInstr& node) override
    {
        result_ = "        mov rax, " + op_emitter_.GetStr( node.first.get()) + "\n"
                  "        mov rbx, " + op_emitter_.GetStr( node.second.get()) + "\n";

        switch ( node.op )
        {
            case ir::BinaryOpType::ADD: result_ += "        add rax, rbx\n"; break;
            case ir::BinaryOpType::SUB: result_ += "        sub rax, rbx\n"; break;
            case ir::BinaryOpType::MUL: result_ += "        mul rax, rbx\n"; break;
            case ir::BinaryOpType::DIV: result_ += "        div rax, rbx\n"; break;
        }

        result_ += "        mov " + op_emitter_.GetStr( node.dest.get()) + ", rax\n";
    }

    void
    Visit( ir::UnaryOpInstr& node) override
    {
        if ( node.op == ir::UnaryOpType::MOV )
        {
            result_ = "        mov rax, " + op_emitter_.GetStr( node.operand.get()) + "\n"
                      "        mov " + op_emitter_.GetStr( node.dest.get()) + ", rax\n";
        } else if ( node.op == ir::UnaryOpType::RET )
        {
            result_ = "        mov rax, " + op_emitter_.GetStr( node.operand.get()) + "\n"
                      "        ret\n";
        }
    }

    void
    Visit( ir::FunctionCallInstr& node) override
    {
        result_ = "";
        for ( auto& it : node.params )
        {
            result_ += "        push " + op_emitter_.GetStr( it.get()) + "\n";
        }
        result_ += "        mov rbp, rsp\n";
                   "        inc rbp\n"
                   "        call FUNC_" + std::to_string( node.func) + "\n"
                   "        mov " + op_emitter_.GetStr( node.dest.get()) + ", rax\n";
    }

    void
    Visit( ir::CmpAndJmpInstr& node) override
    {
        if ( node.type == ir::CmpType::ALWAYS_TRUE )
        {
            result_ = "        jmp .LOC_" + std::to_string( node.true_dest) + "\n";
            return ;
        }

        node.left->Accept( op_emitter_);
        result_ = "        mov rax, " + op_emitter_.GetStr( node.left.get()) + "\n"
                  "        mov rbx, " + op_emitter_.GetStr( node.right.get()) + "\n"
                  "        cmp rax, rbx\n";

        std::string true_label = ".LOC_" + std::to_string( node.true_dest);
        std::string false_label = ".LOC_" + std::to_string( node.false_dest);

        if ( node.type == ir::CmpType::LESS )
        {
            result_ += "        jl " + true_label + "\n"
                       "        jg " + false_label + "\n";
        } else if ( node.type == ir::CmpType::EQUAL )
        {
            result_ += "        je " + true_label + "\n"
                       "        jne " + false_label + "\n";
        } else if ( node.type == ir::CmpType::BIGGER )
        {
            result_ += "        jg " + true_label + "\n"
                       "        jl " + false_label + "\n";
        }

    }

    std::string
    GetStr() const
    {
        return result_;
    }

private:
    std::string result_{};
    OperandEmitter op_emitter_;

};

std::string
EmitBasicBlock( ir::BasicBlock *basic_block,
                const std::unordered_map<ir::VarID, int>& rbp_offsets)
{
    InstructionEmitter emitter{ rbp_offsets};

    std::string result = "    .LOC_" + std::to_string( basic_block->id) + ":\n";
    for ( auto& instr : basic_block->instructions )
    {
        instr.get()->Accept( emitter);
        result += emitter.GetStr();
    }
    return result;
}

std::string
EmitFunction( ir::Function *function)
{
    std::unordered_map<ir::VarID, int> rbp_offsets;

    for ( size_t i = 0; i != function->params.size(); ++i )
    {
        rbp_offsets[function->params[i]] = static_cast<int>( i) + 1;
    }
    for ( size_t i = 0; i != function->variables.size(); ++i )
    {
        std::cout << -(static_cast<int>( i) + 1) << std::endl;
        rbp_offsets[function->variables[i]] = -(static_cast<int>( i) + 1);
    }

    std::string result = "FUNC_" + std::to_string( function->id) + ":\n";

    for ( auto& block : function->basic_blocks )
    {
        std::string block_string = EmitBasicBlock( block.get(), rbp_offsets);
        result += block_string;
    }
    return result;
}

} // ! anonymous namespace

std::string
RunBackend( ir::Program *program)
{
    std::string result = "global __start__\n"
                         "section .text\n"
                         "__start__:\n"
                         "    call FUNC_0\n";
    result += EmitFunction( program->preamble.get());
    result += "        call FUNC_1\n"
              "        mov rax, 60\n"
              "        xor rdi, rdi\n"
              "        syscall\n";

    for ( auto& func : program->functions )
    {
        result += EmitFunction( func.get());
    }

    result += "second .data\n";
    for ( ir::VarID global : program->globals )
    {
        result += "GLOBAL_" + std::to_string( global) + " dq 0\n";
    }
    return result;
}

} // ! namespace dumb
