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

class OperandsInterpreter : public OperandVisitor
{
private:
    void
    Visit( VarOperand& node) override
    {
        result_position_ = &memory_.locals[node.name];
    }

    void
    Visit( ImmOperand& node) override
    {
        result_position_ = &node.value;
    }

    void
    Visit( GVarOperand& node) override
    {
        result_position_ = &memory_.locals[node.name];
    }

public:
    OperandsInterpreter( ProgramMemory& memory)
     :  memory_{ memory}
    {
    }

    int
    GetValue( Operand *op)
    {
        op->Accept( *this);
        return *result_position_;
    }

    void
    PutValue( Operand *op, int value)
    {
        op->Accept( *this);
        *result_position_ = value;
    }

private:
    int *result_position_;
    ProgramMemory& memory_;

};

class Interpreter : public InstructionVisitor
{
private:
    void
    Visit( BinaryOpInstr& node) override
    {
        int first = op_interpreter_.GetValue( node.first.get());
        int second = op_interpreter_.GetValue( node.second.get());

        int result;

        switch ( node.op )
        {
            case BinaryOpType::ADD: result = first + second; break;
            case BinaryOpType::SUB: result = first - second; break;
            case BinaryOpType::MUL: result = first * second; break;
            case BinaryOpType::DIV: result = first / second; break;
        }

        op_interpreter_.PutValue( node.dest.get(), result);
    }

    void
    Visit( UnaryOpInstr& node) override
    {
        if ( node.op == UnaryOpType::MOV )
        {
            int value = op_interpreter_.GetValue( node.operand.get());
            op_interpreter_.PutValue( node.dest.get(), value);
        } else if ( node.op == UnaryOpType::RET )
        {
            func_return_value_ = op_interpreter_.GetValue( node.operand.get());
            need_return_ = true;
        } else
        {
            throw std::runtime_error{ "Unexpected unary operation"};
        }
    }

    void
    Visit( FunctionCallInstr& node) override
    {
        for ( auto& it : node.params )
        {
            params_stack_.emplace_back( op_interpreter_.GetValue( it.get()));
        }
        Function *func = find_function( node.name);
        VisitFunction( *func);
        op_interpreter_.PutValue( node.dest.get(), func_return_value_);
    }

    void
    Visit( CmpAndJmpInstr& node) override
    {
        if ( node.type == CmpType::ALWAYS_TRUE )
        {
            BasicBlock *basic_block = find_basic_block( node.true_dest);
            VisitBasicBlock( *basic_block);
            return ;
        }
        int left  = op_interpreter_.GetValue( node.left.get());
        int right = op_interpreter_.GetValue( node.right.get());
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
            BasicBlock *basic_block = find_basic_block( node.true_dest);
            VisitBasicBlock( *basic_block);
        } else
        {
            BasicBlock *basic_block = find_basic_block( node.false_dest);
            VisitBasicBlock( *basic_block);
        }
    }

    void
    Visit( InputInstr& node) override
    {
        int value;
        std::cout << node.string;
        std::cin >> value;
        op_interpreter_.PutValue( node.dest.get(), value);
    }

    void
    Visit( OutputInstr& node) override
    {
        std::cout << node.string << op_interpreter_.GetValue( node.expression.get()) << std::endl;
    }

    void
    VisitBasicBlock( BasicBlock& node)
    {
        for ( auto& instr : node.instructions )
        {
            instr->Accept( *this);
            if ( need_return_ )
            {
                return ;
            }
        }
    }

    void
    VisitFunction( Function& node)
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
        need_return_ = false;
    }

public:
    void
    RunProgram( Program& program)
    {
        functions_ = &program.functions;
        for ( auto& global : program.globals )
        {
            memory_.globals[global] = 0;
        }
        VisitFunction( *program.preamble.get());

        Function *main = find_function( "main");
        VisitFunction( *main);
    }

private:
    Function *
    find_function( std::string name)
    {
        for ( std::size_t i = 0; i != functions_->size(); ++i )
        {
            if ( functions_->at(i)->name == name )
            {
                return functions_->at( i).get();
            }
        }
        return nullptr;
    }

    BasicBlock *
    find_basic_block( ir::LocalLabelID id)
    {
        for ( std::size_t i = 0; i != basic_blocks_->size(); ++i )
        {
            if ( basic_blocks_->at( i)->id == id )
            {
                return basic_blocks_->at( i).get();
            }
        }
        return nullptr;
    }

private:
    std::vector<FunctionPtr> *functions_;
    std::vector<BasicBlockPtr> *basic_blocks_;
    ProgramMemory memory_{};
    std::vector<int> eval_stack_{};
    std::vector<int> params_stack_{};
    OperandsInterpreter op_interpreter_{ memory_};
    bool need_return_{ false};
    int func_return_value_;

};

} // ! anonymous namespace

void
Run( Program *ir)
{
    Interpreter interpreter{};
    interpreter.RunProgram( *ir);
}

} // ! namespace interpreter
} // ! namespace ir
} // ! namespace dumb

