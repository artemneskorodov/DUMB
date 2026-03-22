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
        result_ = node.name;
    }

    void
    Visit( ir::GVarOperand& node) override
    {
        result_ = node.name;
    }

    void
    Visit( ir::ImmOperand& node) override
    {
        result_ = "imm(" + std::to_string( node.value) + ")";
    }

    std::string
    GetStr( ir::Operand *operand)
    {
        if ( operand == nullptr )
        {
            return "";
        }
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
        std::string left   { op_dumper_.GetStr( node.first.get())};
        std::string right  { op_dumper_.GetStr( node.second.get())};
        std::string dest   { op_dumper_.GetStr( node.dest.get())};
        std::string op_str {};
        switch ( node.op )
        {
            case ir::BinaryOpType::ADD:         op_str = " + "; break;
            case ir::BinaryOpType::SUB:         op_str = " - "; break;
            case ir::BinaryOpType::MUL:         op_str = " * "; break;
            case ir::BinaryOpType::DIV:         op_str = " / "; break;
        }
        result_ = "BinaryOp: " + dest + " = " + left + op_str + right;
    }

    void
    Visit( ir::UnaryOpInstr& node) override
    {
        std::string right  { op_dumper_.GetStr( node.operand.get())};
        std::string dest   { op_dumper_.GetStr( node.dest.get())};
        std::string op_str {};
        switch ( node.op )
        {
            case ir::UnaryOpType::MOV: result_ = "UnaryOp: " + dest + " = " + right; break;
            case ir::UnaryOpType::RET: result_ = "UnaryOp: RET " + right; break;
        }
    }

    void
    Visit( ir::FunctionCallInstr& node) override
    {
        std::string dest = op_dumper_.GetStr( node.dest.get());
        result_ = "FunctionCall: " + dest + " call " + node.name + " (";
        for ( auto& it : node.params )
        {
            std::string param = op_dumper_.GetStr( it.get());
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
        if ( node.type == ir::CmpType::ALWAYS_TRUE )
        {
            result_ = "Jmp: goto BasicBlock_" + std::to_string( node.true_dest);
            return ;
        }

        std::string left_str = op_dumper_.GetStr( node.left.get());
        std::string right_str = op_dumper_.GetStr( node.right.get());
        std::string cmp_str;
        switch ( node.type )
        {
            case ir::CmpType::LESS:   cmp_str = " < ";  break;
            case ir::CmpType::EQUAL:  cmp_str = " == "; break;
            case ir::CmpType::BIGGER: cmp_str = " > ";  break;
            case ir::CmpType::ALWAYS_TRUE: break;
        }

        result_ = "CmpAndJmp: if ( " + left_str + cmp_str + right_str + " ) "
                  "goto BasicBlock_" + std::to_string( node.true_dest) +
                  " else goto BasicBlock_" + std::to_string( node.false_dest);
    }

    void
    Visit( ir::InputInstr& node) override
    {
        result_ = "input (" + op_dumper_.GetStr( node.dest.get()) + ", \"" + node.string + "\")";
    }

    void
    Visit( ir::OutputInstr& node) override
    {
        result_ = "output (" + op_dumper_.GetStr( node.expression.get()) + ", \"" + node.string + "\")";
    }

    std::string
    GetStr( ir::Instruction *instr)
    {
        instr->Accept( *this);
        return result_;
    }

private:
    std::string result_;
    OperandDumper op_dumper_;

};

std::string
dump_basic_block( ir::BasicBlock *basic_block)
{
    InstructionDumper dumper{};

    std::string result = "    BasicBlock_" + std::to_string( basic_block->id) + "\n";

    for ( auto& instr : basic_block->instructions )
    {
        std::string instr_string = dumper.GetStr( instr.get());
        std::cerr << instr_string << std::endl;
        result += "        " + instr_string + "\n";
    }
    return result;
}

std::string
dump_function( ir::Function *function)
{
    std::string result = function->name + " (";
    for ( auto& param : function->params )
    {
        result += param;
        if ( &param != &function->params.back() )
        {
            result += ", ";
        }
    }
    result += ")\n";

    for ( auto& basic_block : function->basic_blocks )
    {
        result += dump_basic_block( basic_block.get()) + "\n";
    }
    return result;
}

} // ! anonymous namespace

void
DumpIR( ir::Program *program)
{
    std::string result = "# Globals:\n";
    for ( const std::string& var : program->globals )
    {
        result += "    " + var + "\n";
    }
    result += "\n# Preamble:\n" + dump_function( program->preamble.get());

    for ( auto& it : program->functions )
    {
        result += dump_function( it.get());
    }
    std::cout << result;
}

} // ! namespace ir_dump
} // ! namespace dumb
