#ifndef DUMP_LIR_HH__
#define DUMP_LIR_HH__

#include <memory>
#include <unordered_map>

#include "ir.hh"

namespace dumb
{
namespace lir
{

using AddrType = std::int32_t;

enum class RegName
{
    RAX,
    RBX,
    RCX,
    RDX,
    RSP,
    RBP,
    RDI
    // TODO add other registers
};

struct Register
{
    explicit
    Register( RegName reg)
     :  reg{ reg}
    {
    }

    RegName reg;

};

struct Immediate
{
    explicit
    Immediate( int value)
     :  value{ value}
    {
    }

    int value;

};

struct RegMem
{
    RegMem( RegName reg,
            AddrType offset)
     :  reg{ reg},
        offset{ offset}
    {
    }

    RegName reg;
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

using Operand = std::variant
<
    Register,
    RegMem,
    Memory,
    Immediate
>;

struct MovInstr
{
    MovInstr( Operand first,
              Operand second)
     :  first{ std::move( first)},
        second{ std::move( second)}
    {
    }

    Operand first;
    Operand second;

};

enum class JmpType
{
    JMP,
    JE,
    JNE,
    JL,
    JG,
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

struct PushInstr
{
    explicit
    PushInstr( Operand operand)
     :  operand{ operand}
    {
    }

    Operand operand;

};

struct PopInstr
{
    explicit
    PopInstr( Operand operand)
     :  operand{ operand}
    {
    }

    Operand operand;

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

struct RetInstr
{
};

enum class MathType
{
    ADD,
    SUB,
    DIV,
    MUL,
    XOR,
    CMP,
};

struct MathInstr
{
    MathInstr( MathType type,
               Operand first,
               Operand second)
     :  type{ type},
        first{ std::move( first)},
        second{ std::move( second)}
    {
    }

    MathType type;
    Operand first;
    Operand second;

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

struct Syscall
{
};

using Instruction = std::variant
<
    MovInstr,
    JmpInstr,
    PushInstr,
    PopInstr,
    CallInstr,
    RetInstr,
    Label,
    MathInstr,
    Syscall
>;

class Program
{
public:
    void
    AddMov( Operand first,
            Operand second)
    {
        instructions_.emplace_back( MovInstr{ first, second});
    }

    void
    AddJmp( JmpType type,
            std::string label)
    {
        instructions_.emplace_back( JmpInstr{ type, std::move( label)});
    }

    void
    AddPush( Operand op)
    {
        instructions_.emplace_back( PushInstr{ op});
    }

    void
    AddPop( Operand op)
    {
        instructions_.emplace_back( PopInstr{ op});
    }

    void
    AddCall( std::string label)
    {
        instructions_.emplace_back( CallInstr{ std::move( label)});
    }

    void
    AddRet()
    {
        instructions_.emplace_back( RetInstr{});
    }

    void
    AddLabel( std::string label)
    {
        instructions_.emplace_back( Label{ std::move( label)});
    }

    void
    AddMath( MathType type,
             Operand first,
             Operand second)
    {
        instructions_.emplace_back( MathInstr{ type, std::move( first), std::move( second)});
    }

    void
    AddSyscall()
    {
        instructions_.emplace_back( Syscall{});
    }

    std::string ToStr() const;

    void AddGlobal( std::string label,
                    int initializer)
    {
        global_labels_.emplace_back( Global{ label, initializer});
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
    std::vector<Instruction> instructions_{};
    std::vector<Global> global_labels_;

};

} // ! namespace lir
} // ! namespace dump

#endif // ! DUMP_LIR_HH__
