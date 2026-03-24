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
mov_to_str( const MovInstr& instr)
{
    return "mov " + operand_to_string( instr.first) + ", " + operand_to_string( instr.second);
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
        default: throw std::runtime_error{ "Unexpected jump type"};
    }
}

std::string
push_to_str( const PushInstr& instr)
{
    return "push " + operand_to_string( instr.operand);
}

std::string
pop_to_str( const PopInstr& instr)
{
    return "pop " + operand_to_string( instr.operand);
}

std::string
call_to_str( const CallInstr& instr)
{
    return "call " + instr.label;
}

std::string
ret_to_str()
{
    return "ret";
}

std::string
math_to_str( const MathInstr& instr)
{
    std::string op_str{};
    switch ( instr.type )
    {
        case MathType::ADD: op_str = "add"; break;
        case MathType::SUB: op_str = "sub"; break;
        case MathType::DIV: op_str = "div"; break;
        case MathType::MUL:
        {
            // TODO move mul and div to another type of instructions (use binary instr and unary instr)
            // Only RAX can be first operand of mul instruction.
            // Result is in RAX
            assert( std::holds_alternative<lir::Register>( instr.first) &&
                    std::get<lir::Register>( instr.first) == lir::Register::RAX);
            return "mul " + operand_to_string( instr.second);
        }
        case MathType::XOR: op_str = "xor"; break;
        case MathType::CMP: op_str = "cmp"; break;
    }
    return op_str + " " + operand_to_string( instr.first) + ", " + operand_to_string( instr.second);
}

std::string
syscall_to_str()
{
    return "syscall";
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
        if ( std::holds_alternative<MovInstr>( instr) )
        {
            result += mov_to_str( std::get<MovInstr>( instr));
        } else if ( std::holds_alternative<JmpInstr>( instr) )
        {
            result += jmp_to_str( std::get<JmpInstr>( instr));
        } else if ( std::holds_alternative<PushInstr>( instr) )
        {
            result += push_to_str( std::get<PushInstr>( instr));
        } else if ( std::holds_alternative<PopInstr>( instr) )
        {
            result += pop_to_str( std::get<PopInstr>( instr));
        } else if ( std::holds_alternative<CallInstr>( instr) )
        {
            result += call_to_str( std::get<CallInstr>( instr));
        } else if ( std::holds_alternative<RetInstr>( instr) )
        {
            result += ret_to_str();
        } else if ( std::holds_alternative<Label>( instr) )
        {
            result += label_to_str( std::get<Label>( instr));
        } else if ( std::holds_alternative<MathInstr>( instr) )
        {
            result += math_to_str( std::get<MathInstr>( instr));
        } else if ( std::holds_alternative<Syscall>( instr) )
        {
            result += syscall_to_str();
        } else
        {
            throw std::runtime_error{ "Unexpected instruction type"};
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
