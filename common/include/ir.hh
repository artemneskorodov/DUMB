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
    virtual void Visit( VarOperand& node) = 0;
    virtual void Visit( ImmOperand& node) = 0;
};

inline void VarOperand::Accept( OperandVisitor& v) { v.Visit( *this); }
inline void ImmOperand::Accept( OperandVisitor& v) { v.Visit( *this); }

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
    CMP_LESS,
    CMP_EQUAL,
    CMP_BIGGER,
};

struct BinaryOpInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    enum class Type
    {
        ADD,
        SUB,
        MUL,
        DIV,
    };

    BinaryOpInstr( VarID dest,
                   BinaryOpType op,
                   OperandPtr first,
                   OperandPtr second)
     :  dest{ dest},
        op{ op},
        first{ std::move( first)},
        second{ std::move( second)}
    {
    }

    VarID dest;
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

    UnaryOpInstr( VarID dest,
                  UnaryOpType op,
                  OperandPtr operand)
     :  dest{ dest},
        op{ op},
        operand{ std::move( operand)}
    {
    }

    VarID dest;
    UnaryOpType op;
    OperandPtr operand;

};

// Function call

using FuncID = std::size_t;

struct FunctionCallInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    FunctionCallInstr( VarID dest,
                       FuncID func,
                       std::vector<OperandPtr> params)
     :  dest{ dest},
        func{ func},
        params{ std::move( params)}
    {
    }

    VarID dest;
    FuncID func;
    std::vector<OperandPtr> params;
};

// Compare and jump instruction

using LocalLabelID = std::size_t;

struct CmpAndJmpInstr : public Instruction
{
    void Accept( InstructionVisitor& v) override;

    CmpAndJmpInstr( OperandPtr cmp_result,
                    LocalLabelID true_dest,
                    LocalLabelID false_dest)
     :  cmp_result{ std::move( cmp_result)},
        true_dest{ true_dest},
        false_dest{ false_dest}
    {
    }

    OperandPtr cmp_result;
    LocalLabelID true_dest;
    LocalLabelID false_dest;

};

class InstructionVisitor
{
public:
    virtual void Visit( BinaryOpInstr& node) = 0;
    virtual void Visit( UnaryOpInstr& node) = 0;
    virtual void Visit( FunctionCallInstr& node) = 0;
    virtual void Visit( CmpAndJmpInstr& node) = 0;

};

inline void BinaryOpInstr::Accept( InstructionVisitor& v) { v.Visit( *this); }
inline void UnaryOpInstr::Accept( InstructionVisitor& v) { v.Visit( *this); }
inline void FunctionCallInstr::Accept( InstructionVisitor& v) { v.Visit( *this); }
inline void CmpAndJmpInstr::Accept( InstructionVisitor& v) { v.Visit( *this); }

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
    std::size_t stack_size{ 0};
    FuncID id;

};

using FunctionPtr = std::unique_ptr<Function>;

//
// Program
//

struct Program final
{
    std::vector<FunctionPtr> functions{};

};

} // ! namespace ir
} // ! namespace dumb

#endif // ! DUMB_IR_HH__
