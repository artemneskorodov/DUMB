#include <unordered_map>
#include <vector>
#include <iostream>

#include "ir_interpreter.hh"
#include "ir.hh"

namespace dumb
{
namespace ir
{
namespace interpreter
{

namespace
{

struct ProgramMemory
{
    std::unordered_map<std::string, int> locals;
    std::unordered_map<std::string, int> globals;

};

class OperandsInterpreter : public ConstantOperandVisitor
{
private:
    void
    Visit( const VarOperand& node) override
    {
        result_position_ = &memory_.locals[node.name];
        result_value_ = *result_position_;
    }

    void
    Visit( const ImmOperand& node) override
    {
        result_position_ = nullptr;
        result_value_ = node.value;
    }

    void
    Visit( const GVarOperand& node) override
    {
        result_position_ = &memory_.locals[node.name];
        result_value_ = *result_position_;
    }

public:
    OperandsInterpreter( ProgramMemory& memory)
     :  memory_{ memory}
    {
    }

    int
    GetValue( const Operand& op)
    {
        op.Accept( *this);
        return result_value_;
    }

    void
    PutValue( const Operand& op, int value)
    {
        op.Accept( *this);
        *result_position_ = value;
    }

private:
    int result_value_;
    int *result_position_;
    ProgramMemory& memory_;

};

class Interpreter : public ConstantInstructionVisitor
{
private:
    void
    Visit( const BinaryOpInstr& node) override
    {
        int first  = op_interpreter_.GetValue( *node.first);
        int second = op_interpreter_.GetValue( *node.second);

        int result;

        switch ( node.op )
        {
            case BinaryOpType::ADD: result = first + second; break;
            case BinaryOpType::SUB: result = first - second; break;
            case BinaryOpType::MUL: result = first * second; break;
            case BinaryOpType::DIV: result = first / second; break;
        }

        op_interpreter_.PutValue( *node.dest, result);
    }

    void
    Visit( const UnaryOpInstr& node) override
    {
        if ( node.op == UnaryOpType::MOV )
        {
            int value = op_interpreter_.GetValue( *node.operand);
            op_interpreter_.PutValue( *node.dest, value);
        } else if ( node.op == UnaryOpType::RET )
        {
            func_return_value_ = op_interpreter_.GetValue( *node.operand);
            need_return_ = true;
        } else
        {
            throw std::runtime_error{ "Unexpected unary operation"};
        }
    }

    void
    Visit( const FunctionCallInstr& node) override
    {
        for ( const OperandPtr& param : node.params )
        {
            params_stack_.emplace_back( op_interpreter_.GetValue( *param));
        }
        const Function& func = find_function( node.name);
        VisitFunction( func);
        op_interpreter_.PutValue( *node.dest, func_return_value_);
    }

    void
    Visit( const CmpAndJmpInstr& node) override
    {
        if ( node.type == CmpType::ALWAYS_TRUE )
        {
            const BasicBlock& basic_block = find_basic_block( node.true_dest);
            VisitBasicBlock( basic_block);
            return ;
        }
        int left  = op_interpreter_.GetValue( *node.left);
        int right = op_interpreter_.GetValue( *node.right);
        bool result;
        switch ( node.type )
        {
            case CmpType::LESS:        result = (left <  right); break;
            case CmpType::EQUAL:       result = (left == right); break;
            case CmpType::BIGGER:      result = (left >  right); break;
            default: throw std::runtime_error{ "Unexpected compare operation"};
        }

        if ( result )
        {
            const BasicBlock& basic_block = find_basic_block( node.true_dest);
            VisitBasicBlock( basic_block);
        } else
        {
            const BasicBlock& basic_block = find_basic_block( node.false_dest);
            VisitBasicBlock( basic_block);
        }
    }

    void
    Visit( const InputInstr& node) override
    {
        int value;
        std::cout << node.string;
        std::cin >> value;
        op_interpreter_.PutValue( *node.dest, value);
    }

    void
    Visit( const OutputInstr& node) override
    {
        std::cout << node.string << op_interpreter_.GetValue( *node.expression) << std::endl;
    }

    void
    VisitBasicBlock( const BasicBlock& node)
    {
        for ( const InstructionPtr& instr : node.instructions )
        {
            instr->Accept( *this);
            if ( need_return_ )
            {
                return ;
            }
        }
    }

    void
    VisitFunction( const Function& node)
    {
        basic_blocks_ = &node.basic_blocks;
        for ( std::size_t i = 0; i != node.params.size(); ++i )
        {
            int value = params_stack_.back();
            params_stack_.pop_back();
            memory_.locals[node.params[i]] = value;
        }
        if ( !params_stack_.empty() )
        {
            throw std::runtime_error{ "Unexpected number of parameters in parameters stack"};
        }

        VisitBasicBlock( *basic_blocks_->at( 0));
        basic_blocks_ = nullptr;
        need_return_ = false;
    }

public:
    void
    RunProgram( const Program& program)
    {
        functions_ = &program.functions;
        for ( auto& global : program.globals )
        {
            memory_.globals[global] = 0;
        }
        VisitFunction( *program.preamble.get());

        const Function& main = find_function( "main");
        VisitFunction( main);
        functions_ = nullptr;
    }

private:
    const Function&
    find_function( std::string name)
    {
        for ( std::size_t i = 0; i != functions_->size(); ++i )
        {
            if ( functions_->at(i)->name == name )
            {
                return *functions_->at( i);
            }
        }
        throw std::runtime_error{ "No function \"" + name + "\""};
    }

    const BasicBlock&
    find_basic_block( ir::LocalLabelID id)
    {
        for ( std::size_t i = 0; i != basic_blocks_->size(); ++i )
        {
            if ( basic_blocks_->at( i)->id == id )
            {
                return *basic_blocks_->at( i);
            }
        }
        throw std::runtime_error{ "No basic block with id = " + std::to_string( id)};
    }

private:
    const std::vector<FunctionPtr>   *functions_;
    const std::vector<BasicBlockPtr> *basic_blocks_;
    ProgramMemory                     memory_{};
    std::vector<int>                  eval_stack_{};
    std::vector<int>                  params_stack_{};
    OperandsInterpreter               op_interpreter_{ memory_};
    bool                              need_return_{ false};
    int                               func_return_value_;

};

} // ! anonymous namespace

void
Run( const Program& ir)
{
    Interpreter interpreter{};
    interpreter.RunProgram( ir);
}

} // ! namespace interpreter
} // ! namespace ir
} // ! namespace dumb

