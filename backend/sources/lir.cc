#include "lir.hh"

namespace dumb
{
namespace lir
{

namespace
{

std::string
reg_to_str( RegName reg)
{
    switch ( reg )
    {
        case RegName::RAX: return "rax";
        case RegName::RBX: return "rbx";
        case RegName::RCX: return "rcx";
        case RegName::RDX: return "rdx";
        case RegName::RSP: return "rsp";
        case RegName::RBP: return "rbx";
    }
}

std::string
operand_to_string( const Operand& operand)
{
    if ( std::holds_alternative<Register>( operand) )
    {
        return reg_to_str( std::get<Register>( operand).reg);
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
        return "[" + reg_to_str( regmem.reg) + sign_str + std::to_string( offset) + "]";
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
        case JmpType::JMP: return "jmp" + instr.label;
        case JmpType::JE:  return "je"  + instr.label;
        case JmpType::JNE: return "jne" + instr.label;
        case JmpType::JL:  return "jl"  + instr.label;
        case JmpType::JG:  return "jg"  + instr.label;
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
        case MathType::MUL: op_str = "mul"; break;
        case MathType::XOR: op_str = "xor"; break;
    }
    return op_str + " " + operand_to_string( instr.first) + ", " + operand_to_string( instr.second);
}

std::string
system_to_str()
{
    return "system";
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
    std::string result{};
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
        } else if ( std::holds_alternative<System>( instr) )
        {
            result += system_to_str();
        } else
        {
            throw std::runtime_error{ "Unexpected instruction type"};
        }
        result += "\n";
    }
}

} // ! namespace lir
} // ! namespace dumb
