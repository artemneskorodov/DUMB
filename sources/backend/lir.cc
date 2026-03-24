#include <stdexcept>
#include <cassert>

#include "lir.hh"

namespace dumb
{
namespace lir
{

namespace
{

inline std::string
reg_to_str( Register reg)
{
    switch ( reg )
    {
        case Register::RAX: return "rax";
        case Register::RBX: return "rbx";
        case Register::RCX: return "rcx";
        case Register::RDX: return "rdx";
        case Register::RSI: return "rsi";
        case Register::RDI: return "rdi";
        case Register::RBP: return "rbp";
        case Register::RSP: return "rsp";
        case Register::R8:  return "r8";
        case Register::R9:  return "r9";
        case Register::R10: return "r10";
        case Register::R11: return "r11";
        case Register::R12: return "r12";
        case Register::R13: return "r13";
        case Register::R14: return "r14";
        case Register::R15: return "r15";
        default: throw std::runtime_error{ "Unexpected register"};
    }
}

std::string
operand_to_string( const Operand& operand)
{
    if ( std::holds_alternative<Register>( operand) )
    {
        return reg_to_str( std::get<Register>( operand));
    } else if ( std::holds_alternative<Memory>( operand) )
    {
        return "[" + std::get<Memory>( operand).label + "]";
    } else if ( std::holds_alternative<RegMem>( operand))
    {
        const RegMem& regmem = std::get<RegMem>( operand);
        std::string sign_str = " + ";
        int offset = regmem.offset;
        if ( offset < 0 )
        {
            offset = -offset;
            sign_str = " - ";
        }
        return "qword [" + reg_to_str( regmem.reg) + sign_str + std::to_string( offset) + "]";
    } else if ( std::holds_alternative<Immediate>( operand) )
    {
        return std::to_string( std::get<Immediate>( operand).value);
    } else if ( std::holds_alternative<StringImm>( operand) )
    {
        return std::get<StringImm>( operand).string;
    } else
    {
        throw std::runtime_error{ "Unexpected operand type"};
    }
}

std::string
jmp_to_str( const JmpInstr& instr)
{
    switch ( instr.type )
    {
        case JmpType::JMP: return "jmp " + instr.label;
        case JmpType::JE:  return "je "  + instr.label;
        case JmpType::JNE: return "jne " + instr.label;
        case JmpType::JL:  return "jl "  + instr.label;
        case JmpType::JG:  return "jg "  + instr.label;
        case JmpType::JLE: return "jle " + instr.label;
        case JmpType::JGE: return "jge " + instr.label;
        default: throw std::runtime_error{ "Unexpected jump type"};
    }
}

std::string
call_to_str( const CallInstr& instr)
{
    return "call " + instr.label;
}

std::string
no_op_to_str( NoOpInstr instr)
{
    switch ( instr )
    {
        case NoOpInstr::SYSCALL: return "syscall";
        case NoOpInstr::RET:     return "ret";
        case NoOpInstr::CQO:     return "cqo";
        default: throw std::runtime_error{ "Unexpected op instruction"};
    }
}

std::string
unary_op_to_str( const UnaryOpInstr& instr)
{
    std::string op_string{};
    switch ( instr.instr )
    {
        case UnaryOp::IMUL: op_string = "imul"; break;
        case UnaryOp::IDIV: op_string = "idiv"; break;
        case UnaryOp::PUSH: op_string = "push"; break;
        case UnaryOp::POP:  op_string =  "pop"; break;
        default: throw std::runtime_error{ "Unexpected op instruction"};
    }

    return op_string + " " + operand_to_string( instr.operand);
}

std::string
binary_op_to_str( const BinaryOpInstr& instr)
{
    std::string op_string{};

    switch( instr.instr )
    {
        case BinaryOp::MOV: op_string = "mov"; break;
        case BinaryOp::ADD: op_string = "add"; break;
        case BinaryOp::SUB: op_string = "sub"; break;
        case BinaryOp::XOR: op_string = "xor"; break;
        case BinaryOp::CMP: op_string = "cmp"; break;
        default: throw std::runtime_error{ "Unexpected op instruction"};
    }

    return op_string + " " + operand_to_string( instr.first) + ", " + operand_to_string( instr.second);
}

std::string
label_to_str( const Label& label)
{
    return label.label + ":";
}

} // ! anonymous namespace

std::string
Program::ToStr() const
{
    std::string result = "global _start:\n"
                         "section .text\n"
                         "extern __std_input\n"
                         "extern __std_output\n";
    for ( const Instruction& instr : instructions_ )
    {
        if ( std::holds_alternative<JmpInstr>( instr) )
        {
            result += jmp_to_str( std::get<JmpInstr>( instr));
        } else if ( std::holds_alternative<CallInstr>( instr) )
        {
            result += call_to_str( std::get<CallInstr>( instr));
        } else if ( std::holds_alternative<Label>( instr) )
        {
            result += label_to_str( std::get<Label>( instr));
        } else if ( std::holds_alternative<NoOpInstr>( instr) )
        {
            result += no_op_to_str( std::get<NoOpInstr>( instr));
        } else if ( std::holds_alternative<UnaryOpInstr>( instr) )
        {
            result += unary_op_to_str( std::get<UnaryOpInstr>( instr));
        } else if ( std::holds_alternative<BinaryOpInstr>( instr) )
        {
            result += binary_op_to_str( std::get<BinaryOpInstr>( instr));
        }
        result += "\n";
    }
    result += "section .data\n";
    for ( const auto& it : global_labels_ )
    {
        result += it.label + " dq " + std::to_string( it.initializer) + "\n";
    }

    for ( const StrConst& str : global_strings_ )
    {
        result += str.label + " db \"" + str.value + "\"\n";
    }
    return result;
}

} // ! namespace lir
} // ! namespace dumb
