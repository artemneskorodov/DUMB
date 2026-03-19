#include <cassert>

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

class Emitter : public ast::Visitor
{
public:
    void
    Visit( ast::Immediate& node) override
    {
        eval_stack_.push_back( std::make_unique<hir::ImmOperand>( node.value));
    }

    void
    Visit( ast::Identifier& node) override
    {
        const nametable::Symbol *sym = nametable_->GetSymbol( node.id);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<hir::VarOperand>( node.id));
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<hir::GVarOperand>( node.id));
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }
    }

    void
    Visit( ast::BinaryOp& node) override
    {
        // Emitting left side of operation
        node.left->Accept( *this);
        hir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Emitting right side of operation
        node.right->Accept( *this);
        hir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Adding instruction
        hir::BinaryOpType type;
        switch ( node.operation )
        {
            case ast::BinaryOp::OP_ADD: type = hir::BinaryOpType::ADD; break;
            case ast::BinaryOp::OP_SUB: type = hir::BinaryOpType::SUB; break;
            case ast::BinaryOp::OP_MUL: type = hir::BinaryOpType::MUL; break;
            case ast::BinaryOp::OP_DIV: type = hir::BinaryOpType::DIV; break;
        }

        hir::VarID tmp_id = tmp_counter_++;
        function_.variables.emplace_back( tmp_id);
        hir::OperandPtr dest = std::make_unique<hir::VarOperand>( tmp_id);
        basic_block_.instructions.emplace_back( std::make_unique<hir::BinaryOpInstr>( std::move( dest), type, std::move( left), std::move( right)));
        eval_stack_.push_back( std::make_unique<hir::VarOperand>( tmp_id));
    }

    void
    Visit( ast::Assignment& node) override
    {
        // Emitting expression
        node.right->Accept( *this);
        hir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        hir::OperandPtr dest = nullptr;
        const nametable::Symbol *sym = nametable_->GetSymbol( node.left);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            dest = std::make_unique<hir::VarOperand>( node.left);
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            dest = std::make_unique<hir::GVarOperand>( node.left);
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        // Adding mov to variable instruction
        basic_block_.instructions.emplace_back( std::make_unique<hir::UnaryOpInstr>( std::move( dest), hir::UnaryOpType::MOV, std::move( expression)));
    }

    void
    Visit( ast::If& node) override
    {
        // Emitting condition

        // Left
        node.condition.left->Accept( *this);
        hir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();
        //Right
        node.condition.right->Accept( *this);
        hir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        hir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = hir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = hir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = hir::CmpType::BIGGER; break;
        }

        // Saving basic basic block which will go after if
        hir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        hir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<hir::CmpAndJmpInstr>( std::move( left), std::move( right), type, true_label, false_label));
        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        // Finishing basic block with new basic block label equals to false label which we saved previously
        finish_basic_block( false_label);
    }

    void
    Visit( ast::While& node) override
    {
        // Adding condition basic block
        finish_basic_block();
        hir::LocalLabelID condition_block = basic_blocks_counter_;

        // Emitting condition
        // Left
        node.condition.left->Accept( *this);
        hir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();
        //Right
        node.condition.right->Accept( *this);
        hir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        hir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = hir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = hir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = hir::CmpType::BIGGER; break;
        }

        hir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        hir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<hir::CmpAndJmpInstr>( std::move( left), std::move( right), type, true_label, false_label));
        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        basic_block_.instructions.emplace_back( std::make_unique<hir::CmpAndJmpInstr>( nullptr, nullptr, hir::CmpType::ALWAYS_TRUE, condition_block, 0));
        finish_basic_block( false_label);
    }

    void
    Visit( ast::FunctionCall& node) override
    {
        std::vector<hir::OperandPtr> parameters;
        for ( auto& it : node.parameters )
        {
            it.get()->Accept( *this);
            parameters.emplace_back( std::move( eval_stack_.back()));
            eval_stack_.pop_back();
        }

        hir::VarID tmp_id = tmp_counter_++;
        function_.variables.emplace_back( tmp_id);
        hir::OperandPtr dest = std::make_unique<hir::VarOperand>( tmp_id);
        basic_block_.instructions.emplace_back( std::make_unique<hir::FunctionCallInstr>( std::move( dest), node.id, std::move( parameters)));
    }

    void
    Visit( ast::Return& node) override
    {
        // Emitting expression to return
        node.expression.get()->Accept( *this);

        hir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        basic_block_.instructions.emplace_back( std::make_unique<hir::UnaryOpInstr>( nullptr /* unused */, hir::UnaryOpType::RET, std::move( expression)));
        finish_basic_block();
    }

    void
    Visit( ast::NewVariable& node) override
    {
        // Counting new variable in stack, adds instructions to basic blocks
        if ( node.initializer != nullptr )
        {
            node.initializer->Accept( *this);
        } else
        {
            eval_stack_.push_back( std::make_unique<hir::ImmOperand>( 0));
        }

        // Adding initialization instruction
        function_.variables.emplace_back( node.identifier);
        hir::OperandPtr initializer = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        hir::OperandPtr dest = nullptr;
        const nametable::Symbol *sym = nametable_->GetSymbol( node.identifier);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            dest = std::make_unique<hir::VarOperand>( node.identifier);
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            program_.globals.emplace_back( node.identifier);
            dest = std::make_unique<hir::GVarOperand>( node.identifier);
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        basic_block_.instructions.push_back( std::make_unique<hir::UnaryOpInstr>( std::move( dest), hir::UnaryOpType::MOV, std::move( initializer)));
    }

    void
    EmitFunction( ast::Function *function)
    {
        start_function( function->id);

        // Getting function parameters
        for ( hir::VarID it : function->parameters )
        {
            function_.params.emplace_back( it);
        }

        // Emitting function body
        for ( auto& it : function->body )
        {
            it.get()->Accept( *this);
        }

        // TODO check if needed
        // Adding last basic block which can be not full to function
        if ( !basic_block_.instructions.empty() )
        {
            finish_basic_block();
        }
        // This is obviously needed
        finish_function();
    }

    void
    EmitProgram( ast::Program *program)
    {
        nametable_ = &program->nametable;

        tmp_counter_ = program->nametable.GetMaxSymbolIndex();

        start_function( 0);

        // Global variables preamble
        for ( auto& it : program->global_variables )
        {
            it->Accept( *this);
        }

        finish_basic_block(); // TODO check if needed
        program_.preamble = std::make_unique<hir::Function>( std::move( function_));

        // Functions
        for ( auto& it : program->functions )
        {
            EmitFunction( &it);
        }
        program_finished_ = true;
    }

    hir::Program
    GetProgram()
    {
        if ( !program_finished_ )
        {
            throw std::runtime_error{ "Expected to run visit on program tree head first"};
        }
        return std::move( program_);
    }

private:
    std::vector<hir::OperandPtr> eval_stack_{};
    hir::VarID tmp_counter_{ 0};

    hir::Program program_{};
    bool program_finished_{ false};
    hir::Function function_{ 0};
    hir::BasicBlock basic_block_{ 0};
    hir::LocalLabelID basic_blocks_counter_{ 0};
    nametable::NameTable *nametable_{ nullptr};

private:
    void
    finish_function()
    {
        program_.functions.emplace_back( std::make_unique<hir::Function>( std::move( function_)));
    }

    void
    start_function( hir::FuncID id)
    {
        function_ = hir::Function{ id};
    }

    void
    finish_basic_block()
    {
        function_.basic_blocks.emplace_back( std::make_unique<hir::BasicBlock>( std::move( basic_block_)));
        basic_block_ = hir::BasicBlock{ ++basic_blocks_counter_};
    }

    void
    finish_basic_block( hir::LocalLabelID next_bb_id)
    {
        function_.basic_blocks.emplace_back( std::make_unique<hir::BasicBlock>( std::move( basic_block_)));
        basic_block_ = hir::BasicBlock{ next_bb_id};
    }

};

} // ! anonymous namespace

hir::Program
EmitIR( ast::Program *program)
{
    Emitter emitter{};

    emitter.EmitProgram( program);

    hir::Program program_ir = emitter.GetProgram();
    return program_ir;
}

} // ! namespace emit_ir
} // ! namespace dumb
