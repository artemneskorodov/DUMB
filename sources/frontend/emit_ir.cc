#include <cassert>
#include <iostream>
#include <stdexcept>

#include "ast.hh"
#include "emit_ir.hh"
#include "ir.hh"
#include "nametable.hh"

namespace dumb
{
namespace emit_ir
{

namespace
{

class Emitter : public ast::ConstantVisitor
{
private:
    void
    Visit( const ast::Immediate& node) override
    {
        assert( eval_stack_.empty());
        eval_stack_.push_back( std::make_unique<ir::ImmOperand>( node.value));
    }

    void
    Visit( const ast::Identifier& node) override
    {
        assert( eval_stack_.empty());
        const nt::Symbol *sym = nametable_->GetSymbol( node.id);
        if ( sym->GetType() == nt::SymbolType::LOCAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<ir::VarOperand>( sym->GetSafeName()));
        } else if ( sym->GetType() == nt::SymbolType::GLOBAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<ir::GVarOperand>( sym->GetSafeName()));
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }
    }

    void
    Visit( const ast::BinaryOp& node) override
    {
        assert( eval_stack_.empty());
        // Emitting left side of operation
        node.left->Accept( *this);
        ir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Emitting right side of operation
        node.right->Accept( *this);
        ir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Adding instruction
        ir::BinaryOpType type;
        switch ( node.operation )
        {
            case ast::BinaryOp::OP_ADD: type = ir::BinaryOpType::ADD; break;
            case ast::BinaryOp::OP_SUB: type = ir::BinaryOpType::SUB; break;
            case ast::BinaryOp::OP_MUL: type = ir::BinaryOpType::MUL; break;
            case ast::BinaryOp::OP_DIV: type = ir::BinaryOpType::DIV; break;
            default: throw std::runtime_error{ "Unexpected binary operation type"};
        }

        std::string tmp_id = "tmp_" + std::to_string( tmp_counter_++);
        function_.variables.emplace_back( tmp_id);
        ir::OperandPtr dest = std::make_unique<ir::VarOperand>( tmp_id);
        ir::InstructionPtr instr = std::make_unique<ir::BinaryOpInstr>( std::move( dest),
                                                                        type,
                                                                        std::move( left),
                                                                        std::move( right));
        basic_block_.instructions.emplace_back( std::move( instr));
        eval_stack_.push_back( std::make_unique<ir::VarOperand>( tmp_id));
    }

    void
    Visit( const ast::Assignment& node) override
    {
        assert( eval_stack_.empty());
        // Emitting expression
        node.right->Accept( *this);
        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        const nt::Symbol *sym = nametable_->GetSymbol( node.left);
        ir::OperandPtr dest = nullptr;
        if ( sym->GetType() == nt::SymbolType::LOCAL_VARIABLE )
        {
            dest = std::make_unique<ir::VarOperand>( sym->GetSafeName());
        } else if ( sym->GetType() == nt::SymbolType::GLOBAL_VARIABLE )
        {
            dest = std::make_unique<ir::GVarOperand>( sym->GetSafeName());
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        ir::InstructionPtr instr = std::make_unique<ir::UnaryOpInstr>( std::move( dest),
                                                                       ir::UnaryOpType::MOV,
                                                                       std::move( expression));
        basic_block_.instructions.emplace_back( std::move( instr));
    }

    void
    Visit( const ast::If& node) override
    {
        assert( eval_stack_.empty());
        // Left
        node.condition.left->Accept( *this);
        ir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();
        //Right
        node.condition.right->Accept( *this);
        ir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = ir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = ir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = ir::CmpType::BIGGER; break;
            default: throw std::runtime_error{ "Unexpected compare operation type"};
        }

        // Saving basic basic block which will go after if
        ir::LocalLabelID true_label = basic_blocks_counter_ + 1;
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2;

        ir::InstructionPtr instr = std::make_unique<ir::CmpAndJmpInstr>( std::move( left),
                                                                         std::move( right),
                                                                         type,
                                                                         true_label,
                                                                         false_label);

        basic_block_.instructions.emplace_back( std::move( instr));
        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        // Finishing basic block with new basic block label equals to false label which we saved previously
        finish_basic_block( false_label);
    }

    void
    Visit( const ast::While& node) override
    {
        assert( eval_stack_.empty());
        // Adding condition basic block
        finish_basic_block();
        ir::LocalLabelID condition_block = basic_blocks_counter_;

        // Emitting condition
        // Left
        node.condition.left->Accept( *this);
        ir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();
        //Right
        node.condition.right->Accept( *this);
        ir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = ir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = ir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = ir::CmpType::BIGGER; break;
            default: throw std::runtime_error{ "Unexpected compare operation type"};
        }

        ir::LocalLabelID true_label = basic_blocks_counter_ + 1;
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        ir::InstructionPtr instr = std::make_unique<ir::CmpAndJmpInstr>( std::move( left),
                                                                         std::move( right),
                                                                         type,
                                                                         true_label,
                                                                         false_label);
        basic_block_.instructions.emplace_back( std::move( instr));

        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        instr = std::make_unique<ir::CmpAndJmpInstr>( nullptr,
                                                      nullptr,
                                                      ir::CmpType::ALWAYS_TRUE,
                                                      condition_block,
                                                      0);

        basic_block_.instructions.emplace_back( std::move( instr));
        finish_basic_block( false_label);
    }

    void
    Visit( const ast::FunctionCall& node) override
    {
        assert( eval_stack_.empty());
        std::vector<ir::OperandPtr> params;
        for ( auto& it : node.parameters )
        {
            it.get()->Accept( *this);
            params.emplace_back( std::move( eval_stack_.back()));
            eval_stack_.pop_back();
        }

        const nt::Symbol *sym = nametable_->GetSymbol( node.id);
        if ( sym->GetType() != nt::SymbolType::FUNCTION )
        {
            throw std::runtime_error{ "Unexpected symbol type"};
        }

        std::string tmp_id = "tmp_" + std::to_string( tmp_counter_++);
        function_.variables.emplace_back( tmp_id);
        ir::OperandPtr dest = std::make_unique<ir::VarOperand>( tmp_id);

        ir::InstructionPtr instr = std::make_unique<ir::FunctionCallInstr>( std::move( dest),
                                                                            sym->GetName(),
                                                                            std::move( params));
        basic_block_.instructions.emplace_back( std::move( instr));

        ir::OperandPtr result = std::make_unique<ir::VarOperand>( tmp_id);
        eval_stack_.push_back( std::move( result));
    }

    void
    Visit( const ast::Return& node) override
    {
        assert( eval_stack_.empty());
        // Emitting expression to return
        node.expression.get()->Accept( *this);

        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::InstructionPtr instr = std::make_unique<ir::UnaryOpInstr>( nullptr,
                                                                       ir::UnaryOpType::RET,
                                                                       std::move( expression));

        basic_block_.instructions.emplace_back( std::move( instr));
        finish_basic_block();
    }

    void
    Visit( const ast::NewVariable& node) override
    {
        assert( eval_stack_.empty());
        // Counting new variable in stack, adds instructions to basic blocks
        if ( node.initializer != nullptr )
        {
            node.initializer->Accept( *this);
        } else
        {
            eval_stack_.push_back( std::make_unique<ir::ImmOperand>( 0));
        }

        ir::OperandPtr initializer = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        const nt::Symbol *sym = nametable_->GetSymbol( node.identifier);
        ir::OperandPtr dest = nullptr;
        if ( sym->GetType() == nt::SymbolType::LOCAL_VARIABLE )
        {
            function_.variables.emplace_back( sym->GetSafeName());
            dest = std::make_unique<ir::VarOperand>( sym->GetSafeName());
        } else if ( sym->GetType() == nt::SymbolType::GLOBAL_VARIABLE )
        {
            program_.globals.emplace_back( sym->GetSafeName());
            dest = std::make_unique<ir::GVarOperand>( sym->GetSafeName());
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        ir::InstructionPtr instr = std::make_unique<ir::UnaryOpInstr>( std::move( dest),
                                                                       ir::UnaryOpType::MOV,
                                                                       std::move( initializer));
        basic_block_.instructions.emplace_back( std::move( instr));
    }

    void
    Visit( const ast::Input& node) override
    {
        assert( eval_stack_.empty());
        const nt::Symbol *sym = nametable_->GetSymbol( node.identifier);

        ir::OperandPtr dest = nullptr;
        if ( sym->GetType() == nt::SymbolType::LOCAL_VARIABLE )
        {
            dest = std::make_unique<ir::VarOperand>( sym->GetSafeName());
        } else if ( sym->GetType() == nt::SymbolType::GLOBAL_VARIABLE )
        {
            dest = std::make_unique<ir::GVarOperand>( sym->GetSafeName());
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        ir::InstructionPtr instr = std::make_unique<ir::InputInstr>( std::move( dest),
                                                                     node.string);
        basic_block_.instructions.emplace_back( std::move( instr));
    };

    void
    Visit( const ast::Output& node) override
    {
        assert( eval_stack_.empty());
        node.expression->Accept( *this);
        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::InstructionPtr instr = std::make_unique<ir::OutputInstr>( std::move( expression),
                                                                      node.string);

        basic_block_.instructions.emplace_back( std::move( instr));
    }

    void
    EmitFunction( const ast::Function& function)
    {
        assert( eval_stack_.empty());
        const nt::Symbol *sym = nametable_->GetSymbol( function.id);
        if ( sym->GetType() != nt::SymbolType::FUNCTION )
        {
            throw std::runtime_error{ "Unexpected symbol type"};
        }
        start_function( sym->GetName());

        // Getting function parameters
        for ( nt::SymbolID param : function.parameters )
        {
            const nt::Symbol *sym = nametable_->GetSymbol( param);
            function_.params.emplace_back( sym->GetSafeName());
        }

        // Emitting function body
        for ( auto& it : function.body )
        {
            it.get()->Accept( *this);
        }

        // Adding last basic block which can be not full to function
        if ( !basic_block_.instructions.empty() )
        {
            finish_basic_block();
        }
        // This is obviously needed
        finish_function();
    }

public:
    ir::Program
    EmitProgram( const ast::Program& program)
    {
        assert( eval_stack_.empty());
        nametable_ = &program.nametable;

        // Preamble
        start_function( "_start");
        for ( const ast::StmtNodePtr& global : program.global_variables )
        {
            global->Accept( *this);
        }
        finish_basic_block();
        program_.preamble = std::make_unique<ir::Function>( std::move( function_));

        // Functions
        for ( const ast::Function& func : program.functions )
        {
            EmitFunction( func);
        }
        nametable_ = nullptr;
        return std::move( program_);
    }

private:
    std::vector<ir::OperandPtr>  eval_stack_           {};
    std::size_t                  tmp_counter_          { 0};
    ir::Program                  program_              {};
    ir::Function                 function_             { ""};
    ir::BasicBlock               basic_block_          { 0};
    ir::LocalLabelID             basic_blocks_counter_ { 0};
    const nt::NameTable         *nametable_            { nullptr};

private:
    void
    finish_function()
    {
        program_.functions.emplace_back( std::make_unique<ir::Function>( std::move( function_)));
    }

    void
    start_function( std::string id)
    {
        function_ = ir::Function{ std::move( id)};
    }

    void
    finish_basic_block()
    {
        ir::BasicBlockPtr bb = std::make_unique<ir::BasicBlock>( std::move( basic_block_));
        function_.basic_blocks.emplace_back( std::move( bb));
        basic_block_ = ir::BasicBlock{ ++basic_blocks_counter_};
    }

    void
    finish_basic_block( ir::LocalLabelID next_bb_id)
    {
        ir::BasicBlockPtr bb = std::make_unique<ir::BasicBlock>( std::move( basic_block_));
        function_.basic_blocks.emplace_back( std::move( bb));
        basic_block_ = ir::BasicBlock{ next_bb_id};
    }

};

} // ! anonymous namespace

ir::Program
EmitIR( const ast::Program& program)
{
    Emitter emitter{};

    ir::Program program_ir = emitter.EmitProgram( program);
    return program_ir;
}

} // ! namespace emit_ir
} // ! namespace dumb
