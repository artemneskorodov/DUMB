#ifndef DUMP_LIR_HH__
#define DUMP_LIR_HH__

#include <memory>
#include <unordered_map>
#include <cstdint>
#include <variant>

#include "ir.hh"

namespace dumb
{
namespace lir
{

using AddrType = std::int32_t;

enum class Register
{
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
};

struct Immediate
{
    Immediate( int value)
     :  value{ value}
    {
    }

    int value;

};

struct RegMem
{
    RegMem( Register reg,
            AddrType offset)
     :  reg{ reg},
        offset{ offset}
    {
    }

    Register reg;
    AddrType offset;

};

struct Memory
{
    explicit
    Memory( std::string label)
     :  label{ std::move( label)}
    {
    }

    std::string label;

};

struct StringImm
{
    explicit
    StringImm( std::string label)
     :  string{ label}
    {
    }

    std::string string;

};

using Operand = std::variant
<
    Register,
    RegMem,
    Memory,
    Immediate,
    StringImm
>;

enum class JmpType
{
    JMP,
    JE,
    JNE,
    JL,
    JG,
    JLE,
    JGE,
};

struct JmpInstr
{
    JmpInstr( JmpType type,
              std::string label)
     :  type{ type},
        label{ std::move( label)}
    {
    }

    JmpType type;
    std::string label;

};

struct CallInstr
{
    explicit
    CallInstr( std::string label)
     :  label{ std::move( label)}
    {
    }

    std::string label;

};

enum class NoOpInstr
{
    SYSCALL,
    RET,
    CQO,
};

enum class UnaryOp
{
    IMUL,
    IDIV,
    PUSH,
    POP,
    DEC
};

struct UnaryOpInstr
{
    explicit
    UnaryOpInstr( UnaryOp instr,
                  Operand operand)
     :  instr   { instr},
        operand { std::move( operand)}
    {
    }

    UnaryOp instr;
    Operand operand;

};

enum class BinaryOp
{
    MOV,
    ADD,
    SUB,
    XOR,
    CMP,
    TEST,
};

struct BinaryOpInstr
{
    BinaryOpInstr( BinaryOp instr,
                   Operand  first,
                   Operand  second)
     :  instr  { instr},
        first  { std::move( first)},
        second { std::move( second)}
    {
    }

    BinaryOp instr;
    Operand  first;
    Operand  second;

};

struct Label
{
    explicit
    Label( std::string label)
     :  label{ std::move( label)}
    {
    }

    std::string label;

};

using Instruction = std::variant
<
    JmpInstr,
    CallInstr,
    Label,
    NoOpInstr,
    UnaryOpInstr,
    BinaryOpInstr
>;

struct StrConst
{
    StrConst( std::string label,
              std::string value)
     :  label{ std::move( label)},
        value{ std::move( value)}
    {
    }

    std::string label;
    std::string value;

};

class Program
{
public:
    void
    Add( NoOpInstr instr)
    {
        instructions_.emplace_back( instr);
    }

    void
    Add( UnaryOp instr,
         Operand operand)
    {
        instructions_.emplace_back( UnaryOpInstr{ instr, std::move( operand)});
    }

    void
    Add( BinaryOp instr,
         Operand  first,
         Operand  second)
    {
        instructions_.emplace_back( BinaryOpInstr{ instr, std::move( first), std::move( second)});
    }

    void
    AddJmp( JmpType type,
            std::string label)
    {
        instructions_.emplace_back( JmpInstr{ type, std::move( label)});
    }

    void
    AddCall( std::string label)
    {
        instructions_.emplace_back( CallInstr{ std::move( label)});
    }

    void
    AddLabel( std::string label)
    {
        instructions_.emplace_back( Label{ std::move( label)});
    }

    std::string ToStr() const;

    void
    AddGlobal( std::string label,
               int initializer)
    {
        global_labels_.emplace_back( Global{ std::move( label), initializer});
    }

    void
    AddStrConst( std::string label,
                 std::string value)
    {
        global_strings_.emplace_back( StrConst{ label, value});
    }

private:
    struct Global
    {
        Global( std::string label,
                int initializer)
         :  label{ label},
            initializer{ initializer}
        {
        }

        std::string label;
        int initializer;

    };

private:
    std::vector<Instruction> instructions_   {};
    std::vector<Global>      global_labels_  {};
    std::vector<StrConst>    global_strings_ {};

};

} // ! namespace lir
} // ! namespace dump

#endif // ! DUMP_LIR_HH__
