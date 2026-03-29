#include <unordered_map>
#include <iostream>

#include "ir.hh"
#include "backend.hh"
#include "lir.hh"

namespace dumb
{

namespace
{


class InstructionsEmitter
{
public:
    InstructionsEmitter( const ir::Program& program)
     :  program_{ program}
    {
    }

    lir::Program
    Emit()
    {
        emit_function( program_.Preamble());
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, lir::Immediate{ 60});
        lir_.Add( lir::BinaryOp::XOR, lir::Register::RDI, lir::Register::RDI);
        lir_.Add( lir::NoOpInstr::SYSCALL);

        // Functions
        for ( const ir::Function& func : program_.Functions() )
        {
            emit_function( func);
        }

        for ( int global : program_.Globals() )
        {
            const nt::Symbol* sym = program_.Nametable().FindSymbol( global);
            lir_.AddGlobal( sym->GetName(), 0);
        }

        for ( std::size_t str_id = 0; str_id != program_.Strings().size(); ++str_id )
        {
            lir_.AddStrConst( string_label( str_id),
                              program_.Strings()[str_id]);
        }
        return std::move( lir_);
    }

private:
    void
    emit_function( const ir::Function& func)
    {
        for ( size_t i = 0; i != func.Params().size(); ++i )
        {
            rbp_offsets_[func.Params()[i]] = 8 * (static_cast<int>( i) + 1);
        }
        for ( size_t i = 0; i != func.Variables().size(); ++i )
        {
            rbp_offsets_[func.Variables()[i]] = - 8 * (static_cast<int>( i) + 1);
        }

        const nt::Symbol *sym = program_.Nametable().FindSymbol( func.Id());
        lir_.AddLabel( sym->GetName());

        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBP, lir::Register::RSP);
        variables_size_ = static_cast<int>( func.Variables().size() * 8);
        lir_.Add( lir::BinaryOp::SUB, lir::Register::RSP, lir::Immediate{ variables_size_});

        for ( const ir::BasicBlock& block : func.BasicBlocks() )
        {
            emit_basic_block( block);
        }
        rbp_offsets_.clear();
    }

    void
    emit_basic_block( const ir::BasicBlock& basic_block)
    {
        lir_.AddLabel( local_label( basic_block.id));

        for ( const ir::Instruction& instr : basic_block.instructions )
        {
            switch ( instr.opcode )
            {
                case ir::Opcode::ADD:    emit_instr_add    ( instr); break;
                case ir::Opcode::SUB:    emit_instr_sub    ( instr); break;
                case ir::Opcode::MUL:    emit_instr_mul    ( instr); break;
                case ir::Opcode::DIV:    emit_instr_div    ( instr); break;
                case ir::Opcode::MOV:    emit_instr_mov    ( instr); break;
                case ir::Opcode::RET:    emit_instr_ret    ( instr); break;
                case ir::Opcode::CALL:   emit_instr_call   ( instr); break;
                case ir::Opcode::INPUT:  emit_instr_input  ( instr); break;
                case ir::Opcode::OUTPUT: emit_instr_output ( instr); break;
                default: throw std::runtime_error{ "Unexpected instruction opcode"};
            }
        }

        emit_terminator( basic_block.terminator);
    }

    void
    emit_terminator( const ir::BasicBlockTerminator& terminator)
    {
        if ( terminator.type == ir::CmpType::INVALID )
        {
            lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, lir::Immediate{ 60});
            lir_.Add( lir::BinaryOp::XOR, lir::Register::RDI, lir::Register::RDI);
            lir_.Add( lir::NoOpInstr::SYSCALL);
            return ;
        }

        if ( terminator.type == ir::CmpType::ALWAYS_TRUE )
        {
            lir_.AddJmp( lir::JmpType::JMP, local_label( terminator.true_dest));
            return ;
        }

        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( terminator.left));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, operand( terminator.right));
        lir_.Add( lir::BinaryOp::CMP, lir::Register::RAX, lir::Register::RBX);

        std::string true_label  = local_label( terminator.true_dest);
        std::string false_label = local_label( terminator.false_dest);

        if ( terminator.type == ir::CmpType::LESS )
        {
            lir_.AddJmp( lir::JmpType::JL, true_label);
            lir_.AddJmp( lir::JmpType::JGE, false_label);
        } else if ( terminator.type == ir::CmpType::EQUAL )
        {
            lir_.AddJmp( lir::JmpType::JE,  true_label);
            lir_.AddJmp( lir::JmpType::JNE, false_label);
        } else if ( terminator.type == ir::CmpType::BIGGER )
        {
            lir_.AddJmp( lir::JmpType::JG, true_label);
            lir_.AddJmp( lir::JmpType::JLE, false_label);
        } else
        {
            throw std::runtime_error{ "Unexpected comparison type"};
        }
    }

    void
    emit_instr_add( const ir::Instruction& instr)
    {
        check_operands( instr, 2);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, operand( instr.operands[1]));
        lir_.Add( lir::BinaryOp::ADD, lir::Register::RAX, lir::Register::RBX);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_sub( const ir::Instruction& instr)
    {
        check_operands( instr, 2);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, operand( instr.operands[1]));
        lir_.Add( lir::BinaryOp::SUB, lir::Register::RAX, lir::Register::RBX);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_mul( const ir::Instruction& instr)
    {
        check_operands( instr, 2);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, operand( instr.operands[1]));
        lir_.Add( lir::UnaryOp::IMUL, lir::Register::RBX);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_div( const ir::Instruction& instr)
    {
        check_operands( instr, 2);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RBX, operand( instr.operands[1]));
        lir_.Add( lir::BinaryOp::XOR, lir::Register::RDX, lir::Register::RDX);
        lir_.Add( lir::NoOpInstr::CQO);
        lir_.Add( lir::UnaryOp::IDIV, lir::Register::RBX);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_mov( const ir::Instruction& instr)
    {
        check_operands( instr, 1);
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_ret( const ir::Instruction& instr)
    {
        if ( instr.operands.size() == 1 )
        {
            lir_.Add( lir::BinaryOp::MOV, lir::Register::RAX, operand( instr.operands[0]));
        } else
        {
            check_operands( instr, 0);
        }

        lir_.Add( lir::BinaryOp::ADD, lir::Register::RSP, lir::Immediate{ variables_size_});
        lir_.Add( lir::NoOpInstr::RET);
    }

    void
    emit_instr_call( const ir::Instruction& instr)
    {
        if ( instr.operands.size() < 1 )
        {
            throw std::runtime_error{ "Unexpected number of operands"};
        }

        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);

        for ( std::size_t param_id = 1; param_id != instr.operands.size(); ++param_id )
        {
            lir_.Add( lir::UnaryOp::PUSH, operand( instr.operands[param_id]));
        }

        int params_num = static_cast<int>( instr.operands.size());

        const nt::Symbol *sym = program_.Nametable().FindSymbol( instr.operands[0].value);

        lir_.AddCall( sym->GetName());
        lir_.Add( lir::BinaryOp::ADD, lir::Register::RSP, lir::Immediate{ 8 * params_num});
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_input( const ir::Instruction& instr)
    {
        check_operands( instr, 1);

        int str_id = instr.operands[0].value;
        std::string str_label = string_label( str_id);
        const std::string& str = program_.Strings()[str_id];

        lir_.Add( lir::BinaryOp::MOV, lir::Register::RSI, lir::StringImm{ str_label});
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RDX, str.length());
        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);
        lir_.AddCall( "__std_input");
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
        lir_.Add( lir::BinaryOp::MOV, operand( instr.defines), lir::Register::RAX);
    }

    void
    emit_instr_output( const ir::Instruction& instr)
    {
        check_operands( instr, 2);

        int str_id = instr.operands[0].value;
        std::string str_label = string_label( str_id);
        const std::string& str = program_.Strings()[str_id];

        lir_.Add( lir::BinaryOp::MOV, lir::Register::RSI, lir::StringImm{ str_label});
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RDX, str.length());
        lir_.Add( lir::BinaryOp::MOV, lir::Register::RCX, operand( instr.operands[1]));
        lir_.Add( lir::UnaryOp::PUSH, lir::Register::RBP);
        lir_.AddCall( "__std_output");
        lir_.Add( lir::UnaryOp::POP, lir::Register::RBP);
    }

private:
    void
    check_operands( const ir::Instruction& instr,
                    std::size_t expected)
    {
        if ( instr.operands.size() != expected )
        {
            throw std::runtime_error{ "Unexpected operands number"};
        }
    }

    lir::Operand
    operand( const ir::Operand& operand)
    {
        switch ( operand.type )
        {
            case ir::Operand::VARIABLE:
            {
                return lir::RegMem{ lir::Register::RBP, rbp_offsets_[operand.value]};
            }
            case ir::Operand::GLOBAL:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.value);
                return lir::Memory{ sym->GetName()};
            }
            case ir::Operand::IMMEDIATE:
            {
                return lir::Immediate{ operand.value};
            }
            default:
            {
                throw std::runtime_error{ "Unexpected operand type"};
            }
        }
    }

    std::string
    string_label( int id)
    {
        return "_GLOBAL_STR_CONST_" + std::to_string( id);
    }

    std::string
    local_label( int id)
    {
        return ".LOC" + std::to_string( id);
    }

private:
    lir::Program lir_{};
    const ir::Program& program_;
    int variables_size_{};
    std::unordered_map<int, int> rbp_offsets_{};

};

} // ! anonymous namespace

std::string
RunBackend( const ir::Program& program)
{
    InstructionsEmitter emitter{ program};
    lir::Program lir = emitter.Emit();
    return lir.ToStr();
}

} // ! namespace dumb
