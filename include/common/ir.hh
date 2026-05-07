#ifndef DUMB_IR_HH__
#define DUMB_IR_HH__

#include <vector>
#include <string>
#include <unordered_map>
#include <list>
#include <stdexcept>

#include "nametable.hh"

namespace dumb
{
namespace ir
{

//
// Operands
//

using ImmType = int;

// Predefinition to use pointers.
struct BasicBlock;

struct Operand
{
    enum Type
    {
        EMPTY,
        VARIABLE,
        GLOBAL,
        IMMEDIATE,
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

    std::string
    ToStr() const
    {
        std::string result = "(type=";
        switch ( type )
        {
            case Type::EMPTY:        result += "EMPTY";        break;
            case Type::VARIABLE:     result += "VARIABLE";     break;
            case Type::GLOBAL:       result += "GLOBAL";       break;
            case Type::IMMEDIATE:    result += "IMMEDIATE";    break;
            case Type::FUNC_LABEL:   result += "FUNC_LABEL";   break;
            case Type::STRING_LABEL: result += "STRING_LABEL"; break;
            default:
            {
                throw std::runtime_error{ "Unexpected Operand::Type value = " +
                                          std::to_string( type)};
            }
        }

        result += ", value=" + std::to_string( value) + ", id=" + std::to_string( id) + ")";
        return result;
    }
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

    SSAKey( const Operand& operand)
     :  id{ operand.id},
        version{ operand.value}
    {
        if ( operand.type != Operand::VARIABLE )
        {
            throw std::runtime_error{ "Unexpected cast from non variable operand to SSAKey"};
        }
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
    EXIT,
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

///
/// @brief BasicBlockID is declared here to use in PhiNode. This is local label index type.
///
using BasicBlockID = int;

struct PhiNode
{
    PhiNode( nt::SymbolID var)
     :  var{ Operand::VARIABLE, 0, var}
    {
    }

    Operand                                   var;
    std::unordered_map<BasicBlockID, Operand> mapping;

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

inline std::string
CmpTypeToStr( CmpType type)
{
    switch ( type )
    {
        case CmpType::LESS:        return "LESS";
        case CmpType::EQUAL:       return "EQUAL";
        case CmpType::BIGGER:      return "BIGGER";
        case CmpType::ALWAYS_TRUE: return "ALWAYS_TRUE";
        case CmpType::INVALID:     return "INVALID";
        default:
        {
            throw std::runtime_error{ "Unexpected CmpType value = " +
                                      std::to_string( static_cast<int>( type))};
        }
    }
}

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

struct BasicBlock final
{
    BasicBlock( BasicBlockID id)
     :  id{ id}
    {
    }

    std::list<PhiNode>      phi_nodes{};
    std::list<Instruction>  instructions{};
    BasicBlockTerminator    terminator{};
    std::list<int>          phi_acceptors{};
    BasicBlockID            id;
    std::list<BasicBlockID> predecessors{};

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
    AddEntryBasicBlock()
    {
        BasicBlock& bb = add_basic_block( current_bb_id_);
        entry_ = current_bb_id_;
        ++current_bb_id_;
        return bb;
    }

    BasicBlock&
    AddBasicBlock()
    {
        if ( entry_ < 0 )
        {
            throw std::runtime_error{ "It is expected to add entry basic block first"};
        }
        return add_basic_block( current_bb_id_++);
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

    BasicBlockID
    Entry() const
    {
        if ( entry_ < 0 )
        {
            throw std::runtime_error{ "Entry is not set"};
        }
        return entry_;
    }

private:
    BasicBlock&
    add_basic_block( ir::BasicBlockID id)
    {
        basic_blocks_.emplace_back( id);
        basic_block_hash_[id] = &basic_blocks_.back();
        return basic_blocks_.back();
    }

private:
    std::vector<SSAKey>                            params_           {};
    std::list<BasicBlock>                          basic_blocks_     {};
    std::unordered_map<BasicBlockID, BasicBlock *> basic_block_hash_ {};
    std::list<SSAKey>                              variables_        {};
    int                                            id_;
    BasicBlockID                                   current_bb_id_    { 0};
    BasicBlockID                                   entry_            { -1};

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
        functions_hash_[id] = &functions_.back();
        return functions_.back();
    }

    Function&
    GetFunction( int id) &
    {
        return *functions_hash_.at( id);
    }

    const Function&
    GetFunction( int id) const &
    {
        return *functions_hash_.at( id);
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

    const std::list<Function>&
    Functions() const &
    {
        return functions_;
    }

    std::list<Function>&
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

    void
    SetEntry( int entry)
    {
        entry_ = entry;
    }

    int
    Entry() const
    {
        if ( entry_ < 0 )
        {
            throw std::runtime_error{ "Entry is not set"};
        }
        return entry_;
    }

private:
    nt::NameTable                       nametable_;
    std::list<Function>                 functions_      {};
    std::unordered_map<int, Function *> functions_hash_ {};
    std::vector<std::string>            strings_        {};
    std::vector<int>                    globals_        {};
    int                                 entry_          { -1};

};

} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_HH__
