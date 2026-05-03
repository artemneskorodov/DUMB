#ifndef DUMB_IR_HH__
#define DUMB_IR_HH__

#include <vector>
#include <string>
#include <unordered_map>
#include <list>

#include "nametable.hh"

namespace dumb
{
namespace ir
{

//
// Operands
//

using ImmType = int;

struct Operand
{
    enum Type
    {
        EMPTY,
        VARIABLE,
        GLOBAL,
        IMMEDIATE,
        LABEL,
        FUNC_LABEL,
        STRING_LABEL,
    };

    constexpr
    Operand( Type type,
             ImmType value,
             nt::SymbolID id = 0)
     :  type{ type},
        value{ value},
        id{ id}
    {
    }

    constexpr
    Operand()
     :  type{ EMPTY},
        value{},
        id{}
    {
    }

    Type type;
    ImmType value; // version for variable
    nt::SymbolID id;

};

constexpr Operand kNoDefine = Operand{};

//
// Keys and hashed used for mapping
//

struct SSAKey
{
    SSAKey( nt::SymbolID id,
            ImmType version)
     :  id{ id},
        version{ version}
    {
    }

    bool
    operator==( const SSAKey& other) const
    {
        return (id == other.id) && (version == other.version);
    }

    nt::SymbolID id;
    ImmType version;

};

struct SSAKeyHash
{
    std::size_t
    operator()( const SSAKey& key) const
    {
        return std::hash<int>()( key.version ^ (key.id << 1));
    }
};

//
// Instructions
//

enum class Opcode
{
    ADD,
    SUB,
    MUL,
    DIV,
    MOV,
    RET,
    CALL,
    INPUT,
    OUTPUT,
};

struct Instruction
{
    Instruction( Opcode opcode,
                 const Operand& defines,
                 std::vector<Operand> operands)
     :  opcode   { opcode},
        defines  { defines},
        operands { std::move( operands)}
    {
    }

    Opcode opcode;
    Operand defines;
    std::vector<Operand> operands;

};

struct PhiNode
{
    PhiNode( int var)
    :  var_id{ var}
    {
    }

    nt::SymbolID                     var_id;
    int                              version;
    std::unordered_map<int, Operand> mapping;

};

//
// Basic block
//

enum class CmpType
{
    LESS,
    EQUAL,
    BIGGER,
    ALWAYS_TRUE,
    INVALID,
};

struct BasicBlockTerminator
{
    BasicBlockTerminator() = default;

    BasicBlockTerminator( Operand      left,
                          Operand      right,
                          CmpType      type,
                          int          true_dest,
                          int          false_dest)
     :  left       { std::move( left)},
        right      { std::move( right)},
        type       { type},
        true_dest  { true_dest},
        false_dest { false_dest}
    {
    }

    Operand   left       {};
    Operand   right      {};
    CmpType   type       { CmpType::INVALID};
    int       true_dest  { 0xffffff}; // Random number for default constructor
    int       false_dest { 0};

};

using BasicBlockID = int;

struct BasicBlock final
{
    BasicBlock( BasicBlockID id)
     :  id{ id}
    {
    }

    std::list<PhiNode>     phi_nodes{};
    std::list<Instruction> instructions{};
    BasicBlockTerminator   terminator{};
    std::list<int>         phi_acceptors{};
    BasicBlockID           id;

};

//
// Function
//

class Function final
{
public:
    explicit
    Function( int id)
     :  id_{ id}
    {
    }

    BasicBlock&
    AddBasicBlock( BasicBlockID id)
    {
        basic_blocks_.emplace_back( id);
        basic_block_hash_[id] = &basic_blocks_.back();
        return basic_blocks_.back();
    }

    BasicBlock&
    GetBasicBlock( BasicBlockID id) &
    {
        return *basic_block_hash_.at( id);
    }

    const BasicBlock&
    GetBasicBlock( BasicBlockID id) const &
    {
        return *basic_block_hash_.at( id);
    }

    void
    AddVariable( nt::SymbolID id, int version)
    {
        variables_.emplace_back( id, version);
    }

    void
    RemoveVariable( nt::SymbolID id, int version)
    {
        variables_.remove( SSAKey{ id, version});
    }

    void
    AddParam( nt::SymbolID id, int version)
    {
        params_.emplace_back( id, version);
    }

    const std::vector<SSAKey>&
    Params() const &
    {
        return params_;
    }

    const std::list<BasicBlock>&
    BasicBlocks() const &
    {
        return basic_blocks_;
    }

    std::list<BasicBlock>&
    BasicBlocks() &
    {
        return basic_blocks_;
    }

    const std::list<SSAKey>&
    Variables() const &
    {
        return variables_;
    }

    int
    Id() const
    {
        return id_;
    }

private:
    std::vector<SSAKey>                            params_       {};
    std::list<BasicBlock>                          basic_blocks_ {};
    std::unordered_map<BasicBlockID, BasicBlock *> basic_block_hash_{};
    std::list<SSAKey>                              variables_    {};
    int                                            id_;

};

//
// Program
//

class Program final
{
public:
    Program( nt::NameTable nametable)
     :  nametable_{ std::move( nametable)}
    {
    }

    Function&
    AddFunction( int id) &
    {
        functions_.emplace_back( id);
        return functions_.back();
    }

    Function&
    GetFunction( int id) &
    {
        return functions_[id];
    }

    const Function&
    GetFunction( int id) const &
    {
        return functions_[id];
    }

    nt::NameTable&
    Nametable() &
    {
        return nametable_;
    }

    const nt::NameTable&
    Nametable() const &
    {
        return nametable_;
    }

    int
    AddString( std::string str)
    {
        strings_.emplace_back( std::move( str));
        return strings_.size() - 1;
    }

    const std::vector<Function>&
    Functions() const &
    {
        return functions_;
    }

    std::vector<Function>&
    Functions() &
    {
        return functions_;
    }

    void
    AddGlobal( int id)
    {
        globals_.emplace_back( id);
    }

    const std::vector<int>&
    Globals() const &
    {
        return globals_;
    }

    const std::vector<std::string>&
    Strings() const &
    {
        return strings_;
    }

private:
    nt::NameTable            nametable_;
    std::vector<Function>    functions_ {};
    std::vector<std::string> strings_   {};
    std::vector<int>         globals_{};

};

} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_HH__
