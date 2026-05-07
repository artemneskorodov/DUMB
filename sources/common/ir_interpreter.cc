#include <unordered_map>
#include <vector>
#include <iostream>

#include "ir_interpreter.hh"
#include "ir.hh"
#include "logger.hh"

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
        run_function( program_.GetFunction( program_.Entry()));
        if ( !need_exit_ )
        {
            throw std::runtime_error{ "No exit instruction found. UB."};
        }
    }

private:
    void
    run_function( const Function& func)
    {
        LOGGER(IR_INTERPRETER) << "Running function " << func.Id();

        const Function* old_func = current_function_;
        current_function_ = &func;
        for ( const SSAKey& param : func.Params() )
        {
            int value = params_stack_.back();
            params_stack_.pop_back();
            locals_[param] = value;
        }

        if ( !params_stack_.empty() )
        {
            throw std::runtime_error{ "Unexpected number of parameters in parameters stack"};
        }

        run_basic_block( func.GetBasicBlock( func.Entry()));
        need_return_ = false;
        current_function_ = old_func;
    }

    void
    run_basic_block( const BasicBlock& basic_block,
                     BasicBlockID      from_id = -1)
    {
        LOGGER(IR_INTERPRETER) << "Running basic block " << basic_block.id;

        if ( from_id >= 0 )
        {
            for ( const PhiNode& phi : basic_block.phi_nodes )
            {
                Operand dest = phi.var;
                Operand src = phi.mapping.at( from_id);
                ImmType src_val = value( src);

                LOGGER(IR_INTERPRETER) << "Running Phi node: " << dest.ToStr() << " = "
                                       << src.ToStr() << " = " << src_val;

                storage( dest) = src_val;
            }
        }

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
                case Opcode::EXIT:   run_instr_exit   ( instr); break;
                default: throw std::runtime_error{ "Unexpected instruction opcode"};
            }

            if ( need_return_ )
            {
                return ;
            }
            if ( need_exit_ )
            {
                return ;
            }
        }

        if ( basic_block.terminator.type == CmpType::INVALID )
        {
            LOGGER(IR_INTERPRETER) << "BB_" << basic_block.id << " terminator is invalid";
            return ;
        }

        if ( basic_block.terminator.type == CmpType::ALWAYS_TRUE )
        {
            LOGGER(IR_INTERPRETER) << "BB_" << basic_block.id << " terminator is always true";
            const BasicBlock& dest = find_basic_block( basic_block.terminator.true_dest);
            run_basic_block( dest, basic_block.id);
            return ;
        }

        ImmType left  = value( basic_block.terminator.left);
        ImmType right = value( basic_block.terminator.right);
        bool result;
        switch ( basic_block.terminator.type )
        {
            case CmpType::LESS:        result = (left <  right); break;
            case CmpType::EQUAL:       result = (left == right); break;
            case CmpType::BIGGER:      result = (left >  right); break;
            default: throw std::runtime_error{ "Unexpected compare operation"};
        }

        LOGGER(IR_INTERPRETER) << "BB_" << basic_block.id << " terminator: (" << left << " "
                               << CmpTypeToStr( basic_block.terminator.type)
                               << " " << right << ") = " << (result ? "true" : "false");

        if ( result )
        {
            const BasicBlock& dest = find_basic_block( basic_block.terminator.true_dest);
            run_basic_block( dest, basic_block.id);
        } else
        {
            const BasicBlock& dest = find_basic_block( basic_block.terminator.false_dest);
            run_basic_block( dest, basic_block.id);
        }
    }

    void
    run_instr_add( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction add";

        ImmType result = value( instr.operands[0]) + value( instr.operands[1]);

        LOGGER(IR_INTERPRETER) << instr.defines.ToStr() + " = "
                               << instr.operands[0].ToStr() + " + "
                               << instr.operands[1].ToStr() + " = "
                               << result;

        storage( instr.defines) = result;
    }

    void
    run_instr_sub( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction sub";

        ImmType result = value( instr.operands[0]) - value( instr.operands[1]);

        LOGGER(IR_INTERPRETER) << instr.defines.ToStr() + " = "
                               << instr.operands[0].ToStr() + " - "
                               << instr.operands[1].ToStr() + " = "
                               << result;

        storage( instr.defines) = result;
    }

    void
    run_instr_mul( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction mul";

        ImmType result = value( instr.operands[0]) * value( instr.operands[1]);

        LOGGER(IR_INTERPRETER) << instr.defines.ToStr() + " = "
                               << instr.operands[0].ToStr() + " * "
                               << instr.operands[1].ToStr() + " = "
                               << result;

        storage( instr.defines) = result;
    }

    void
    run_instr_div( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction div";

        ImmType result = value( instr.operands[0]) / value( instr.operands[1]);

        LOGGER(IR_INTERPRETER) << instr.defines.ToStr() + " = "
                               << instr.operands[0].ToStr() + " / "
                               << instr.operands[1].ToStr() + " = "
                               << result;

        storage( instr.defines) = result;
    }

    void
    run_instr_mov( const Instruction& instr)
    {
        ImmType val = value( instr.operands[0]);

        LOGGER(IR_INTERPRETER) << "Running instruction mov: " << instr.defines.ToStr() << " = "
                               << instr.operands[0].ToStr() << " = " << val;

        storage( instr.defines) = val;
    }

    void
    run_instr_ret( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction ret";

        if ( instr.operands.size() == 1 )
        {
            ImmType val = value( instr.operands[0]);

            LOGGER(IR_INTERPRETER) << "Retval = " << instr.operands[0].ToStr() << " = " << val;

            function_retval_ = val;
        }
        need_return_ = true;
    }

    void
    run_instr_call( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction call";

        size_t params_number = instr.operands.size() - 1;
        params_stack_.resize( params_number);

        for ( size_t i = 0; i != params_number; ++i )
        {
            ImmType val = value( instr.operands[i + 1]);

            LOGGER(IR_INTERPRETER) << "Pushing " << instr.operands[i + 1].ToStr() << " = " << val;

            params_stack_[params_number - i - 1] = val;
        }

        const Function& func = find_function( instr.operands[0].id);
        LOGGER(IR_INTERPRETER) << "Calling function " << func.Id();

        std::unordered_map<SSAKey, int, SSAKeyHash> locals_old = locals_;
        run_function( func);
        locals_ = locals_old;

        if ( instr.defines.type != Operand::EMPTY )
        {
            LOGGER(IR_INTERPRETER) << "Got return value: " << instr.defines.ToStr() << " = "
                                   << function_retval_;

            storage( instr.defines) = function_retval_;
        }
    }

    void
    run_instr_input( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction input";

        const std::string& str = program_.Strings()[instr.operands[0].value];
        std::cout << str;

        int value;
        std::cin >> value;
        storage( instr.defines) = value;
    }

    void
    run_instr_output( const Instruction& instr)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction output";

        const std::string& str = program_.Strings()[instr.operands[0].value];
        std::cout << str + std::to_string( value( instr.operands[1])) << std::endl;
    }

    void
    run_instr_exit( const Instruction& /*instr*/)
    {
        LOGGER(IR_INTERPRETER) << "Running instruction EXIT";

        need_exit_ = true;
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
    find_basic_block( BasicBlockID id)
    {
        return current_function_->GetBasicBlock( id);
    }

    int
    value( const Operand& operand)
    {
        switch ( operand.type )
        {
            case Operand::VARIABLE:  return locals_[SSAKey{ operand.id, operand.value}];
            case Operand::GLOBAL:    return globals_[operand.id];
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
            case Operand::VARIABLE: return locals_[SSAKey{ operand.id, operand.value}];
            case Operand::GLOBAL:   return globals_[operand.id];
            default: throw std::runtime_error{ "Unexpected operand type = " +
                                               std::to_string( static_cast<int>( operand.type))};
        }
    }

private:
    const Program&                                   program_;
    std::vector<ImmType>                             params_stack_{};
    std::unordered_map<SSAKey, ImmType, SSAKeyHash>  locals_;
    std::unordered_map<nt::SymbolID, ImmType>        globals_;
    const Function                                  *current_function_;
    ImmType                                          function_retval_;
    bool                                             need_return_ { false};
    bool                                             need_exit_   { false};

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

