#ifndef DUMB_IR_HH__
#define DUMB_IR_HH__

#include <vector>

namespace dumb
{
namespace ir
{

//
// Operands
//

class OperandVisitor;

struct Operand
{
    virtual ~Operand() = default;
    virtual void Accept( OperandVisitor& v) = 0;

};

using OperandPtr = std::unique_ptr<Operand>;

// Variable

using VarID = std::size_t;

struct VarOperand : public Operand
{
    void Accept( OperandVisitor& v) override;

    VarOperand( VarID id)
     :  id{ id}
    {
    }

    VarID id;

};

struct GVarOperand : public Operand
{
    void Accept( OperandVisitor& v) override;

    GVarOperand( VarID id)
     :  id{ id}
    {
    }

    VarID id;
};

// Immediate

using ImmType = int;

struct ImmOperand : public Operand
{
    void Accept( OperandVisitor& v) override;

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

inline void VarOperand::Accept( OperandVisitor& v) { v.Visit( *this); }
inline void ImmOperand::Accept( OperandVisitor& v) { v.Visit( *this); }
inline void GVarOperand::Accept( OperandVisitor& v) { v.Visit( *this); }

//
// Instructions
//

class InstructionVisitor;

struct Instruction
{
    virtual ~Instruction() = default;
    virtual void Accept( InstructionVisitor& v) = 0;

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

    BinaryOpInstr( OperandPtr dest,
                   BinaryOpType op,
                   OperandPtr first,
                   OperandPtr second)
     :  dest{ std::move( dest)},
        op{ op},
        first{ std::move( first)},
        second{ std::move( second)}
    {
    }

    OperandPtr dest;
    BinaryOpType op;
    OperandPtr first;
    OperandPtr second;

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

    UnaryOpInstr( OperandPtr dest,
                  UnaryOpType op,
                  OperandPtr operand)
     :  dest{ std::move( dest)},
        op{ op},
        operand{ std::move( operand)}
    {
    }

    OperandPtr dest;
    UnaryOpType op;
    OperandPtr operand;

};

// Function call

using FuncID = std::size_t;

struct FunctionCallInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    FunctionCallInstr( OperandPtr dest,
                       FuncID func,
                       std::vector<OperandPtr> params)
     :  dest{ std::move( dest)},
        func{ func},
        params{ std::move( params)}
    {
    }

    OperandPtr dest;
    FuncID func;
    std::vector<OperandPtr> params;
};

// Compare and jump instruction

using LocalLabelID = std::size_t;

enum class CmpType
{
    LESS,
    EQUAL,
    BIGGER,
    ALWAYS_TRUE,
};

struct CmpAndJmpInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    CmpAndJmpInstr( OperandPtr left,
                    OperandPtr right,
                    CmpType type,
                    LocalLabelID true_dest,
                    LocalLabelID false_dest)
     :  left{ std::move( left)},
        right{ std::move( right)},
        type{ type},
        true_dest{ true_dest},
        false_dest{ false_dest}
    {
    }

    OperandPtr left;
    OperandPtr right;
    CmpType type;
    LocalLabelID true_dest;
    LocalLabelID false_dest;

};

struct InputInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    InputInstr( OperandPtr dest,
                std::string string)
     :  dest{ std::move( dest)},
        string{ std::move( string)}
    {
    }

    OperandPtr dest;
    std::string string;

};

struct OutputInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    OutputInstr( OperandPtr expression,
                 std::string string)
     :  expression{ std::move( expression)},
        string{ std::move( string)}
    {
    }

    OperandPtr expression;
    std::string string;

};

class InstructionVisitor
{
public:
    virtual void Visit( BinaryOpInstr&     node) = 0;
    virtual void Visit( UnaryOpInstr&      node) = 0;
    virtual void Visit( FunctionCallInstr& node) = 0;
    virtual void Visit( CmpAndJmpInstr&    node) = 0;
    virtual void Visit( InputInstr&        node) = 0;
    virtual void Visit( OutputInstr&       node) = 0;

};

inline void BinaryOpInstr::Accept     ( InstructionVisitor& v) { v.Visit( *this); }
inline void UnaryOpInstr::Accept      ( InstructionVisitor& v) { v.Visit( *this); }
inline void FunctionCallInstr::Accept ( InstructionVisitor& v) { v.Visit( *this); }
inline void CmpAndJmpInstr::Accept    ( InstructionVisitor& v) { v.Visit( *this); }
inline void InputInstr::Accept        ( InstructionVisitor& v) { v.Visit( *this); }
inline void OutputInstr::Accept       ( InstructionVisitor& v) { v.Visit( *this); }

//
// Basic block
//

struct BasicBlock final
{
    BasicBlock( LocalLabelID id)
     :  id{ id}
    {
    }

    std::vector<InstructionPtr> instructions{};
    LocalLabelID id;

};

using BasicBlockPtr = std::unique_ptr<BasicBlock>;

//
// Function
//

struct Function final
{
    Function( FuncID id)
     :  id{ id}
    {
    }

    std::vector<VarID> params{};
    std::vector<BasicBlockPtr> basic_blocks{};
    std::vector<VarID> variables{};
    FuncID id;

};

using FunctionPtr = std::unique_ptr<Function>;

//
// Program
//

struct Program final
{
    std::vector<FunctionPtr> functions{};
    FunctionPtr preamble{ nullptr};
    std::vector<VarID> globals{};
    std::vector<std::string> strings{};

};

} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_HH__
