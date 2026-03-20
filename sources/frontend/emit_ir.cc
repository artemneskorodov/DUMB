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
        eval_stack_.push_back( std::make_unique<ir::ImmOperand>( node.value));
    }

    void
    Visit( ast::Identifier& node) override
    {
        const nametable::Symbol *sym = nametable_->GetSymbol( node.id);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<ir::VarOperand>( node.id));
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            eval_stack_.push_back( std::make_unique<ir::GVarOperand>( node.id));
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
        }

        ir::VarID tmp_id = tmp_counter_++;
        function_.variables.emplace_back( tmp_id);
        ir::OperandPtr dest = std::make_unique<ir::VarOperand>( tmp_id);
        basic_block_.instructions.emplace_back( std::make_unique<ir::BinaryOpInstr>( std::move( dest), type, std::move( left), std::move( right)));
        eval_stack_.push_back( std::make_unique<ir::VarOperand>( tmp_id));
    }

    void
    Visit( ast::Assignment& node) override
    {
        // Emitting expression
        node.right->Accept( *this);
        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::OperandPtr dest = nullptr;
        const nametable::Symbol *sym = nametable_->GetSymbol( node.left);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            dest = std::make_unique<ir::VarOperand>( node.left);
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            dest = std::make_unique<ir::GVarOperand>( node.left);
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        // Adding mov to variable instruction
        basic_block_.instructions.emplace_back( std::make_unique<ir::UnaryOpInstr>( std::move( dest), ir::UnaryOpType::MOV, std::move( expression)));
    }

    void
    Visit( ast::If& node) override
    {
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
        }

        // Saving basic basic block which will go after if
        ir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( std::move( left), std::move( right), type, true_label, false_label));
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
        }

        ir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( std::move( left), std::move( right), type, true_label, false_label));
        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( nullptr, nullptr, ir::CmpType::ALWAYS_TRUE, condition_block, 0));
        finish_basic_block( false_label);
    }

    void
    Visit( ast::FunctionCall& node) override
    {
        std::vector<ir::OperandPtr> parameters;
        for ( auto& it : node.parameters )
        {
            it.get()->Accept( *this);
            parameters.emplace_back( std::move( eval_stack_.back()));
            eval_stack_.pop_back();
        }

        ir::VarID tmp_id = tmp_counter_++;
        function_.variables.emplace_back( tmp_id);
        ir::OperandPtr dest = std::make_unique<ir::VarOperand>( tmp_id);
        basic_block_.instructions.emplace_back( std::make_unique<ir::FunctionCallInstr>( std::move( dest), node.id, std::move( parameters)));
    }

    void
    Visit( ast::Return& node) override
    {
        // Emitting expression to return
        node.expression.get()->Accept( *this);

        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        basic_block_.instructions.emplace_back( std::make_unique<ir::UnaryOpInstr>( nullptr /* unused */, ir::UnaryOpType::RET, std::move( expression)));
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
            eval_stack_.push_back( std::make_unique<ir::ImmOperand>( 0));
        }

        // Adding initialization instruction
        function_.variables.emplace_back( node.identifier);
        ir::OperandPtr initializer = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::OperandPtr dest = nullptr;
        const nametable::Symbol *sym = nametable_->GetSymbol( node.identifier);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            dest = std::make_unique<ir::VarOperand>( node.identifier);
        } else if ( sym->GetType() == nametable::Symbol::Type::GLOBAL_VARIABLE )
        {
            program_.globals.emplace_back( node.identifier);
            dest = std::make_unique<ir::GVarOperand>( node.identifier);
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }

        basic_block_.instructions.emplace_back( std::make_unique<ir::UnaryOpInstr>( std::move( dest), ir::UnaryOpType::MOV, std::move( initializer)));
    }

    void
    Visit( ast::Input& node) override
    {
        ir::OperandPtr dest;
        const nametable::Symbol *sym = nametable_->GetSymbol( node.identifier);
        if ( sym->GetType() == nametable::Symbol::Type::LOCAL_VARIABLE )
        {
            dest = std::make_unique<ir::VarOperand>( node.identifier);
        } else
        {
            dest = std::make_unique<ir::GVarOperand>( node.identifier);
        }

        basic_block_.instructions.emplace_back( std::make_unique<ir::InputInstr>( std::move( dest), node.string));
    };

    void
    Visit( ast::Output& node) override
    {
        node.expression->Accept( *this);
        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        basic_block_.instructions.emplace_back( std::make_unique<ir::OutputInstr>( std::move( expression), node.string));
    }

    void
    EmitFunction( ast::Function *function)
    {
        start_function( function->id);

        // Getting function parameters
        for ( ir::VarID it : function->parameters )
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
        program_.preamble = std::make_unique<ir::Function>( std::move( function_));

        // Functions
        for ( auto& it : program->functions )
        {
            EmitFunction( &it);
        }
        program_finished_ = true;
    }

    ir::Program
    GetProgram()
    {
        if ( !program_finished_ )
        {
            throw std::runtime_error{ "Expected to run visit on program tree head first"};
        }
        return std::move( program_);
    }

private:
    std::vector<ir::OperandPtr> eval_stack_{};
    ir::VarID tmp_counter_{ 0};

    ir::Program program_{};
    bool program_finished_{ false};
    ir::Function function_{ 0};
    ir::BasicBlock basic_block_{ 0};
    ir::LocalLabelID basic_blocks_counter_{ 0};
    nametable::NameTable *nametable_{ nullptr};

private:
    void
    finish_function()
    {
        program_.functions.emplace_back( std::make_unique<ir::Function>( std::move( function_)));
    }

    void
    start_function( ir::FuncID id)
    {
        function_ = ir::Function{ id};
    }

    void
    finish_basic_block()
    {
        function_.basic_blocks.emplace_back( std::make_unique<ir::BasicBlock>( std::move( basic_block_)));
        basic_block_ = ir::BasicBlock{ ++basic_blocks_counter_};
    }

    void
    finish_basic_block( ir::LocalLabelID next_bb_id)
    {
        function_.basic_blocks.emplace_back( std::make_unique<ir::BasicBlock>( std::move( basic_block_)));
        basic_block_ = ir::BasicBlock{ next_bb_id};
    }

};

} // ! anonymous namespace

ir::Program
EmitIR( ast::Program *program)
{
    Emitter emitter{};

    emitter.EmitProgram( program);

    ir::Program program_ir = emitter.GetProgram();
    return program_ir;
}

} // ! namespace emit_ir
} // ! namespace dumb
