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
    std::unordered_map<int, int> locals;
    std::unordered_map<int, int> globals;

};


class Interpreter
{
public:
    Interpreter( const Program& ir)
     :  program_{ ir}
    {
    }

    void
    Run()
    {
        run_function( program_.Preamble());
    }

private:
    void
    run_function( const Function& func)
    {
        current_function_ = &func;

        for ( int param : func.Params() )
        {
            int value = params_stack_.back();
            params_stack_.pop_back();
            locals_[param] = value;
        }
        if ( !params_stack_.empty() )
        {
            throw std::runtime_error{ "Unexpected number of parameters in parameters stack"};
        }

        run_basic_block( func.BasicBlocks()[0]);
        need_return_ = false;
    }

    void
    run_basic_block( const BasicBlock& basic_block)
    {
        for ( const Instruction& instr : basic_block.instructions )
        {
            switch ( instr.opcode )
            {
                case Opcode::ADD:    run_instr_add    ( instr); break;
                case Opcode::SUB:    run_instr_sub    ( instr); break;
                case Opcode::MUL:    run_instr_mul    ( instr); break;
                case Opcode::DIV:    run_instr_div    ( instr); break;
                case Opcode::MOV:    run_instr_mov    ( instr); break;
                case Opcode::RET:    run_instr_ret    ( instr); break;
                case Opcode::CALL:   run_instr_call   ( instr); break;
                case Opcode::INPUT:  run_instr_input  ( instr); break;
                case Opcode::OUTPUT: run_instr_output ( instr); break;
                default: throw std::runtime_error{ "Unexpected instruction opcode"};
            }

            if ( need_return_ )
            {
                return ;
            }
        }

        if ( basic_block.terminator.type == CmpType::INVALID )
        {
            return ;
        }

        if ( basic_block.terminator.type == CmpType::ALWAYS_TRUE )
        {
            const BasicBlock& dest = find_basic_block( basic_block.terminator.true_dest);
            run_basic_block( dest);
            return ;
        }

        int left  = value( basic_block.terminator.left);
        int right = value( basic_block.terminator.right);
        bool result;
        switch ( basic_block.terminator.type )
        {
            case CmpType::LESS:        result = (left <  right); break;
            case CmpType::EQUAL:       result = (left == right); break;
            case CmpType::BIGGER:      result = (left >  right); break;
            default: throw std::runtime_error{ "Unexpected compare operation"};
        }

        if ( result )
        {
            const BasicBlock& dest = find_basic_block( basic_block.terminator.true_dest);
            run_basic_block( dest);
        } else
        {
            const BasicBlock& dest = find_basic_block( basic_block.terminator.false_dest);
            run_basic_block( dest);
        }
    }

    void
    run_instr_add( const Instruction& instr)
    {
        int result = value( instr.operands[0]) + value( instr.operands[1]);
        storage( instr.defines) = result;
    }

    void
    run_instr_sub( const Instruction& instr)
    {
        int result = value( instr.operands[0]) - value( instr.operands[1]);
        storage( instr.defines) = result;
    }

    void
    run_instr_mul( const Instruction& instr)
    {
        int result = value( instr.operands[0]) * value( instr.operands[1]);
        storage( instr.defines) = result;
    }

    void
    run_instr_div( const Instruction& instr)
    {
        int result = value( instr.operands[0]) / value( instr.operands[1]);
        storage( instr.defines) = result;
    }

    void
    run_instr_mov( const Instruction& instr)
    {
        storage( instr.defines) = value( instr.operands[0]);
    }

    void
    run_instr_ret( const Instruction& instr)
    {
        if ( instr.operands.size() == 1 )
        {
            function_retval_ = value( instr.operands[0]);
        }
        need_return_ = true;
    }

    void
    run_instr_call( const Instruction& instr)
    {
        size_t params_number = instr.operands.size() - 1;
        params_stack_.resize( params_number);

        for ( size_t i = 0; i != params_number; ++i )
        {
            params_stack_[params_number - i - 1] = value( instr.operands[i + 1]);
        }
        const Function& func = find_function( instr.operands[0].value);
        run_function( func);
        if ( instr.defines.type != Operand::EMPTY )
        {
            storage( instr.defines) = function_retval_;
        }
    }

    void
    run_instr_input( const Instruction& instr)
    {
        const std::string& str = program_.Strings()[instr.operands[0].value];
        std::cout << str;

        int value;
        std::cin >> value;
        storage( instr.defines) = value;
    }

    void
    run_instr_output( const Instruction& instr)
    {
        const std::string& str = program_.Strings()[instr.operands[0].value];
        std::cout << str + std::to_string( value( instr.operands[1])) << std::endl;
    }

private:
    const Function&
    find_function( int id)
    {
        for ( const Function& func : program_.Functions() )
        {
            if ( func.Id() == id )
            {
                return func;
            }
        }
        throw std::runtime_error{ "No function with id\"" + std::to_string( id) + "\""};
    }

    const BasicBlock&
    find_basic_block( int id)
    {
        for ( const BasicBlock& bb : current_function_->BasicBlocks() )
        {
            if ( bb.id == id )
            {
                return bb;
            }
        }
        throw std::runtime_error{ "No basic block with id = " + std::to_string( id)};
    }

    int
    value( const Operand& operand)
    {
        switch ( operand.type )
        {
            case Operand::VARIABLE:  return locals_[operand.value];
            case Operand::GLOBAL:    return globals_[operand.value];
            case Operand::IMMEDIATE: return operand.value;
            default: throw std::runtime_error{ "Unexpected operand type = " +
                                               std::to_string( static_cast<int>( operand.type))};
        }
    }

    int&
    storage( const Operand& operand)
    {
        switch ( operand.type )
        {
            case Operand::VARIABLE: return locals_[operand.value];
            case Operand::GLOBAL:   return globals_[operand.value];
            default: throw std::runtime_error{ "Unexpected operand type = " +
                                               std::to_string( static_cast<int>( operand.type))};
        }
    }

private:
    const Program& program_;
    std::vector<int> params_stack_{};
    std::unordered_map<int, int> locals_;
    std::unordered_map<int, int> globals_;
    const Function *current_function_;
    int function_retval_;
    bool need_return_{ false};

};

} // ! anonymous namespace

void
Run( const Program& ir)
{
    Interpreter interpreter{ ir};
    interpreter.Run();
}

} // ! namespace interpreter
} // ! namespace ir
} // ! namespace dumb

