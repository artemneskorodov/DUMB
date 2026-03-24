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
    explicit
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

struct AddInstr
{
    AddInstr( Operand first,
              Operand second)
     :  first  { std::move( first)},
        second { std::move( second)}
    {
    }

    Operand first;
    Operand second;

};

struct SubInstr
{
    SubInstr( Operand first,
              Operand second)
     :  first  { std::move( first)},
        second { std::move( second)}
    {
    }

    Operand first;
    Operand second;

};

struct XorInstr
{
    XorInstr( Operand first,
              Operand second)
     :  first  { std::move( first)},
        second { std::move( second)}
    {
    }

    Operand first;
    Operand second;

};

struct CmpInstr
{
    CmpInstr( Operand first,
              Operand second)
     :  first  { std::move( first)},
        second { std::move( second)}
    {
    }

    Operand first;
    Operand second;

};

struct MulInstr
{
    explicit
    MulInstr( Operand mult)
     :  mult { std::move( mult)}
    {
    }

    Operand mult;

};

struct DivInstr
{
    explicit
    DivInstr( Operand divider)
     :  divider { std::move( divider)}
    {
    }

    Operand divider;

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

struct Cqo
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
    AddInstr,
    SubInstr,
    XorInstr,
    CmpInstr,
    MulInstr,
    DivInstr,
    Syscall,
    Cqo
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
    AddAdd( Operand first,
            Operand second)
    {
        instructions_.emplace_back( AddInstr{ std::move( first), std::move( second)});
    }

    void
    AddSub( Operand first,
            Operand second)
    {
        instructions_.emplace_back( SubInstr{ std::move( first), std::move( second)});
    }

    void
    AddXor( Operand first,
            Operand second)
    {
        instructions_.emplace_back( XorInstr{ std::move( first), std::move( second)});
    }

    void
    AddCmp( Operand first,
            Operand second)
    {
        instructions_.emplace_back( CmpInstr{ std::move( first), std::move( second)});
    }

    void
    AddMul( Operand mult)
    {
        instructions_.emplace_back( MulInstr{ std::move( mult)});
    }

    void
    AddDiv( Operand divider)
    {
        instructions_.emplace_back( DivInstr{ std::move( divider)});
    }

    void
    AddSyscall()
    {
        instructions_.emplace_back( Syscall{});
    }

    void
    AddCqo()
    {
        instructions_.emplace_back( Cqo{});
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
