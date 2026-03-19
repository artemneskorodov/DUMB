#ifndef DUMP_LIR_HH__
#define DUMP_LIR_HH__

#include <memory>
#include <unordered_map>

#include "ir.hh"

namespace dumb
{
namespace lir
{

enum class RegName
{
    RAX,
    RBX,
    RCX,
    RDX,
    RSP,
    RBP,
    // TODO add other registers
};

using AddrType = std::size_t;

struct Register
{
    explicit
    Register( RegName reg)
     :  reg{ reg}
    {
    }

    RegName reg;

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
    Memory( AddrType offset)
     :  offset{ offset}
    {
    }

    AddrType offset;

};

using Operand = std::variant
<
    Register,
    RegMem,
    Memory
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

// TODO add jump type
struct JmpInstr
{
    explicit
    JmpInstr( AddrType offset)
     :  offset{ offset}
    {
    }

    AddrType offset;

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
    CallInstr( AddrType addr)
     :  addr{ addr}
    {
    }

    AddrType addr;

};

struct RetInstr
{
};

using Instruction = std::variant
<
    MovInstr,
    JmpInstr,
    PushInstr,
    PopInstr,
    CallInstr,
    RetInstr
>;

class Program
{
public:
    void
    Mov( Operand first, Operand second)
    {
        instructions_.emplace_back( MovInstr{ first, second});
        if ( std::holds_alternative<Memory>( first) )
        {
            Memory& memory = std::get<Memory>( std::get<MovInstr>( instructions_.back()).first);
            fixups_positions_.emplace_back( &memory.offset);
        }
        if ( std::holds_alternative<Memory>( second) )
        {
            Memory& memory = std::get<Memory>( std::get<MovInstr>( instructions_.back()).second);
            fixups_positions_.emplace_back( &memory.offset);
        }
    }

    void
    Jmp( AddrType id_of_label)
    {
        instructions_.emplace_back( JmpInstr{ id_of_label});
        fixups_positions_.emplace_back( &std::get<JmpInstr>( instructions_.back()).offset);
    }

    void
    Push( Operand op)
    {
        instructions_.emplace_back( PushInstr{ op});
        if ( std::holds_alternative<Memory>( op) )
        {
            Memory& memory = std::get<Memory>( std::get<PushInstr>( instructions_.back()).operand);
            fixups_positions_.emplace_back( &memory.offset);
        }
    }

    void
    Pop( Operand op)
    {
        instructions_.emplace_back( PopInstr{ op});
        if ( std::holds_alternative<Memory>( op) )
        {
            Memory& memory = std::get<Memory>( std::get<PopInstr>( instructions_.back()).operand);
            fixups_positions_.emplace_back( &memory.offset);
        }
    }

    void
    Call( AddrType id_of_label)
    {
        instructions_.emplace_back( CallInstr{ id_of_label});
        fixups_positions_.emplace_back( &std::get<CallInstr>( instructions_.back()).addr);
    }

    void
    Ret()
    {
        instructions_.emplace_back( RetInstr{});
    }

    void
    Label( AddrType id_of_label)
    {
        AddrType addr = instructions_.size() - 1;
        fixups_map_[id_of_label] = addr;
    }

    void
    Finalizer()
    {
        for ( AddrType *fixup : fixups_positions_ )
        {
            AddrType result = fixups_map_[*fixup];
            *fixup = result;
        }
    }

private:
    std::vector<Instruction>               instructions_     {};
    std::vector<AddrType *>                fixups_positions_ {};
    std::unordered_map<AddrType, AddrType> fixups_map_       {};

};

} // ! namespace lir
} // ! namespace dump

#endif // ! DUMP_LIR_HH__
