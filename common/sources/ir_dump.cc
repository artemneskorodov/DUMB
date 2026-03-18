#include <iostream>

#include "ir_dump.hh"
#include "ir.hh"

namespace dumb
{
namespace ir_dump
{

namespace
{

class OperandDumper : public ir::OperandVisitor
{
public:
    void
    Visit( ir::VarOperand& node) override
    {
        result_ = "tmp_" + std::to_string( node.id);
    }

    void
    Visit( ir::ImmOperand& node) override
    {
        result_ = "imm(" + std::to_string( node.value) + ")";
    }

    std::string
    GetStr( ir::Operand *operand)
    {
        operand->Accept( *this);
        return result_;
    }

private:
    std::string result_;

};

class InstructionDumper : public ir::InstructionVisitor
{
public:
    void
    Visit( ir::BinaryOpInstr& node) override
    {
        std::string left = operand_dumper_.GetStr( node.first.get());
        std::string right = operand_dumper_.GetStr( node.second.get());
        std::string op_str = "";
        switch ( node.op )
        {
            case ir::BinaryOpType::ADD:         op_str = " + "; break;
            case ir::BinaryOpType::SUB:         op_str = " - "; break;
            case ir::BinaryOpType::MUL:         op_str = " * "; break;
            case ir::BinaryOpType::DIV:         op_str = " / "; break;
            case ir::BinaryOpType::CMP_LESS:    op_str = " < "; break;
            case ir::BinaryOpType::CMP_EQUAL:   op_str = " == "; break;
            case ir::BinaryOpType::CMP_BIGGER:  op_str = " > "; break;
        }
        result_ = "BinaryOp: tmp_" + std::to_string( node.dest) + " = " + left + op_str + right;
    }

    void
    Visit( ir::UnaryOpInstr& node) override
    {
        std::string right = operand_dumper_.GetStr( node.operand.get());
        std::string op_str = "";
        switch ( node.op )
        {
            case ir::UnaryOpType::MOV: result_ = "UnaryOp: tmp_" + std::to_string( node.dest) + " = " + right; break;
            case ir::UnaryOpType::RET: result_ = "UnaryOp: RET " + right; break;
        }
    }

    void
    Visit( ir::FunctionCallInstr& node) override
    {
        result_ = "FunctionCall: tmp_" + std::to_string( node.dest) + " call " + std::to_string( node.func) + " (";
        for ( auto& it : node.params )
        {
            std::string param = operand_dumper_.GetStr( it.get());
            result_ += param;
            if ( &it != &node.params.back() )
            {
                result_ += ", ";
            }
        }
        result_ += ")";
    }

    void
    Visit( ir::CmpAndJmpInstr& node) override
    {
        if ( node.cmp_result != nullptr )
        {
            std::string cmp_result = operand_dumper_.GetStr( node.cmp_result.get());
            result_ = "CmpAndJmp: if (" + cmp_result + ") goto BasicBlock_" + std::to_string( node.true_dest) + " else goto BasicBlock_" + std::to_string( node.false_dest);
        } else
        {
            result_ = "CmpAndJmp: goto BasicBlock_" + std::to_string( node.true_dest);
        }
    }

    std::string
    GetStr( ir::Instruction *instr)
    {
        instr->Accept( *this);
        return result_;
    }

private:
    std::string result_;
    OperandDumper operand_dumper_;

};

std::string
dump_basic_block( ir::BasicBlock *basic_block)
{
    InstructionDumper dumper{};

    std::string result = "    BasicBlock_" + std::to_string( basic_block->id) + "\n";

    for ( auto& instr : basic_block->instructions )
    {
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl;
        std::string instr_string = dumper.GetStr( instr.get());
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl;
        std::cerr << instr_string << std::endl;
        result += "        " + instr_string + "\n";
    }
    return result;
}

std::string
dump_function( ir::Function *function)
{
    std::string result = "function_" + std::to_string( function->id) + " (";
    for ( auto& param : function->params )
    {
        result += "tmp_" + std::to_string( param);
        if ( &param != &function->params.back() )
        {
            result += ", ";
        }
    }
    result += ")\n";

    for ( auto& basic_block : function->basic_blocks )
    {
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl;
        result += dump_basic_block( basic_block.get()) + "\n";
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl;
    }
    return result;
}

} // ! anonymous namespace

void
DumpIR( ir::Program *program)
{
    std::string result = "";
    for ( auto& it : program->functions )
    {
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl;
        result += dump_function( it.get());
    }
    std::cout << result;
}

} // ! namespace ir_dump
} // ! namespace dumb
