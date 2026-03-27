#ifndef DUMB_IR_HH__
#define DUMB_IR_HH__

#include <vector>
#include <memory>
#include <string>

namespace dumb
{
namespace ir
{

//
// Operands
//

class OperandVisitor;
class ConstantOperandVisitor;

struct Operand
{
    virtual ~Operand() = default;
    virtual void Accept( OperandVisitor& v) = 0;
    virtual void Accept( ConstantOperandVisitor& v) const = 0;

};

using OperandPtr = std::unique_ptr<Operand>;

// Variable

struct VarOperand : public Operand
{
    void Accept( OperandVisitor& v) override;
    void Accept( ConstantOperandVisitor& v) const override;

    explicit
    VarOperand( std::string name)
     :  name{ std::move( name)}
    {
    }

    std::string name;

};

// Global variable

struct GVarOperand : public Operand
{
    void Accept( OperandVisitor& v) override;
    void Accept( ConstantOperandVisitor& v) const override;

    explicit
    GVarOperand( std::string name)
     :  name{ std::move( name)}
    {
    }

    std::string name;

};

// Immediate

using ImmType = int;

struct ImmOperand : public Operand
{
    void Accept( OperandVisitor& v) override;
    void Accept( ConstantOperandVisitor& v) const override;

    explicit
    ImmOperand( ImmType value)
     :  value{ value}
    {
    }

    ImmType value;

};

class OperandVisitor
{
public:
    virtual void Visit( VarOperand&  node) = 0;
    virtual void Visit( ImmOperand&  node) = 0;
    virtual void Visit( GVarOperand& node) = 0;

};

class ConstantOperandVisitor
{
public:
    virtual void Visit( const VarOperand&  node) = 0;
    virtual void Visit( const ImmOperand&  node) = 0;
    virtual void Visit( const GVarOperand& node) = 0;

};

inline void VarOperand::Accept  ( OperandVisitor& v) { v.Visit( *this); }
inline void ImmOperand::Accept  ( OperandVisitor& v) { v.Visit( *this); }
inline void GVarOperand::Accept ( OperandVisitor& v) { v.Visit( *this); }

inline void VarOperand::Accept  ( ConstantOperandVisitor& v) const { v.Visit( *this); }
inline void ImmOperand::Accept  ( ConstantOperandVisitor& v) const { v.Visit( *this); }
inline void GVarOperand::Accept ( ConstantOperandVisitor& v) const { v.Visit( *this); }

//
// Instructions
//

class InstructionVisitor;
class ConstantInstructionVisitor;

struct Instruction
{
    virtual ~Instruction() = default;
    virtual void Accept( InstructionVisitor& v) = 0;
    virtual void Accept( ConstantInstructionVisitor &v) const = 0;

};

using InstructionPtr = std::unique_ptr<Instruction>;

// Binary operation

enum class BinaryOpType
{
    ADD,
    SUB,
    MUL,
    DIV,
};

struct BinaryOpInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;
    void Accept( ConstantInstructionVisitor& v) const override;

    BinaryOpInstr( OperandPtr dest,
                   BinaryOpType op,
                   OperandPtr first,
                   OperandPtr second)
     :  dest   { std::move( dest)},
        op     { op},
        first  { std::move( first)},
        second { std::move( second)}
    {
    }

    OperandPtr   dest;
    BinaryOpType op;
    OperandPtr   first;
    OperandPtr   second;

};

// Unary operation

enum class UnaryOpType
{
    MOV,
    RET, // dest is unused for return
};

struct UnaryOpInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;
    void Accept( ConstantInstructionVisitor& v) const override;

    UnaryOpInstr( OperandPtr dest,
                  UnaryOpType op,
                  OperandPtr operand)
     :  dest    { std::move( dest)},
        op      { op},
        operand { std::move( operand)}
    {
    }

    OperandPtr  dest;
    UnaryOpType op;
    OperandPtr  operand;

};

// Function call

struct FunctionCallInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;
    void Accept( ConstantInstructionVisitor& v) const override;

    FunctionCallInstr( OperandPtr              dest,
                       std::string             name,
                       std::vector<OperandPtr> params)
     :  dest   { std::move( dest)},
        name   { std::move( name)},
        params { std::move( params)}
    {
    }

    OperandPtr              dest;
    std::string             name;
    std::vector<OperandPtr> params;

};

// User input instruction

struct InputInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;
    void Accept( ConstantInstructionVisitor& v) const override;

    InputInstr( OperandPtr  dest,
                std::string string)
     :  dest   { std::move( dest)},
        string { std::move( string)}
    {
    }

    OperandPtr  dest;
    std::string string;

};

// User output instruction

struct OutputInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;
    void Accept( ConstantInstructionVisitor& v) const override;

    OutputInstr( OperandPtr  expression,
                 std::string string)
     :  expression { std::move( expression)},
        string     { std::move( string)}
    {
    }

    OperandPtr  expression;
    std::string string;

};

class InstructionVisitor
{
public:
    virtual void Visit( BinaryOpInstr&     node) = 0;
    virtual void Visit( UnaryOpInstr&      node) = 0;
    virtual void Visit( FunctionCallInstr& node) = 0;
    virtual void Visit( InputInstr&        node) = 0;
    virtual void Visit( OutputInstr&       node) = 0;

};

class ConstantInstructionVisitor
{
public:
    virtual void Visit( const BinaryOpInstr&     node) = 0;
    virtual void Visit( const UnaryOpInstr&      node) = 0;
    virtual void Visit( const FunctionCallInstr& node) = 0;
    virtual void Visit( const InputInstr&        node) = 0;
    virtual void Visit( const OutputInstr&       node) = 0;

};

inline void BinaryOpInstr::Accept     ( InstructionVisitor& v) { v.Visit( *this); }
inline void UnaryOpInstr::Accept      ( InstructionVisitor& v) { v.Visit( *this); }
inline void FunctionCallInstr::Accept ( InstructionVisitor& v) { v.Visit( *this); }
inline void InputInstr::Accept        ( InstructionVisitor& v) { v.Visit( *this); }
inline void OutputInstr::Accept       ( InstructionVisitor& v) { v.Visit( *this); }

inline void BinaryOpInstr::Accept     ( ConstantInstructionVisitor& v) const { v.Visit( *this); }
inline void UnaryOpInstr::Accept      ( ConstantInstructionVisitor& v) const { v.Visit( *this); }
inline void FunctionCallInstr::Accept ( ConstantInstructionVisitor& v) const { v.Visit( *this); }
inline void InputInstr::Accept        ( ConstantInstructionVisitor& v) const { v.Visit( *this); }
inline void OutputInstr::Accept       ( ConstantInstructionVisitor& v) const { v.Visit( *this); }

//
// Basic block
//

using LocalLabelID = std::size_t;

enum class CmpType
{
    LESS,
    EQUAL,
    BIGGER,
    ALWAYS_TRUE,
};

struct BasicBlockTerminator
{
    BasicBlockTerminator() = default;

    BasicBlockTerminator( OperandPtr   left,
                          OperandPtr   right,
                          CmpType      type,
                          LocalLabelID true_dest,
                          LocalLabelID false_dest)
     :  left       { std::move( left)},
        right      { std::move( right)},
        type       { type},
        true_dest  { true_dest},
        false_dest { false_dest}
    {
    }

    OperandPtr   left       { nullptr};
    OperandPtr   right      { nullptr};
    CmpType      type       { CmpType::ALWAYS_TRUE};
    LocalLabelID true_dest  { 0xffffff}; // Random number for default constructor
    LocalLabelID false_dest { 0};

};

struct BasicBlock final
{
    BasicBlock( LocalLabelID id)
     :  id{ id}
    {
    }

    std::vector<InstructionPtr> instructions{};
    BasicBlockTerminator        terminator{};
    std::vector<LocalLabelID>   predecessors{};
    LocalLabelID                id;

};

using BasicBlockPtr = std::unique_ptr<BasicBlock>;

//
// Function
//

struct Function final
{
    explicit
    Function( std::string name)
     :  name{ std::move( name)}
    {
    }

    std::vector<std::string>   params       {};
    std::vector<BasicBlockPtr> basic_blocks {};
    std::vector<std::string>   variables    {};
    std::string                name;

};

using FunctionPtr = std::unique_ptr<Function>;

//
// Program
//

struct Program final
{
    std::vector<FunctionPtr> functions {};
    FunctionPtr              preamble  { nullptr};
    std::vector<std::string> globals   {};
    std::vector<std::string> strings   {};

};

} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_HH__
