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
        eval_stack_.push_back( std::make_unique<ir::VarOperand>( node.id));
    }

    void
    Visit( ast::BinaryOp& node) override
    {
        // Emitting left side of operation
        node.left.get()->Accept( *this);
        ir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Emitting right side of operation
        node.right.get()->Accept( *this);
        ir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Adding comparison instruction
        ir::BinaryOpType type;
        switch ( node.operation )
        {
            case ast::BinaryOp::OP_ADD: type = ir::BinaryOpType::ADD; break;
            case ast::BinaryOp::OP_SUB: type = ir::BinaryOpType::SUB; break;
            case ast::BinaryOp::OP_MUL: type = ir::BinaryOpType::MUL; break;
            case ast::BinaryOp::OP_DIV: type = ir::BinaryOpType::DIV; break;
        }

        ir::VarID tmp_id = tmp_counter_++;
        basic_block_.instructions.emplace_back( std::make_unique<ir::BinaryOpInstr>( tmp_id, type, std::move( left), std::move( right)));
        eval_stack_.push_back( std::make_unique<ir::VarOperand>( tmp_id));
    }

    void
    Visit( ast::CompareOp& node) override
    {
        // Emitting left side of comparison
        node.left.get()->Accept( *this);
        ir::OperandPtr left = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Emitting right side of comparison
        node.right.get()->Accept( *this);
        ir::OperandPtr right = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Adding comparison instruction
        ir::BinaryOpType type;
        switch ( node.operation )
        {
            case ast::CompareOp::OP_CMP_LESS:   type = ir::BinaryOpType::CMP_LESS;   break;
            case ast::CompareOp::OP_CMP_EQUAL:  type = ir::BinaryOpType::CMP_EQUAL;  break;
            case ast::CompareOp::OP_CMP_BIGGER: type = ir::BinaryOpType::CMP_BIGGER; break;
        }

        ir::VarID tmp_id = tmp_counter_++;
        basic_block_.instructions.emplace_back( std::make_unique<ir::BinaryOpInstr>( tmp_counter_, type, std::move( left), std::move( right)));
        eval_stack_.push_back( std::make_unique<ir::VarOperand>( tmp_id));
    }

    void
    Visit( ast::Assignment& node) override
    {
        // Emitting expression
        node.right.get()->Accept( *this);
        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Getting left hand side variable id
        // FIXME save variable index in assignment node, as it is the only thing that can occur there
        node.left.get()->Accept( *this);
        const ir::VarOperand *lhs = static_cast<const ir::VarOperand *>( eval_stack_.back().get());
        ir::VarID lhs_id = lhs->id;
        eval_stack_.pop_back();

        // Adding mov to variable instruction
        basic_block_.instructions.emplace_back( std::make_unique<ir::UnaryOpInstr>( lhs_id, ir::UnaryOpType::MOV, std::move( expression)));
    }

    void
    Visit( ast::If& node) override
    {
        // Emitting condition
        node.condition.get()->Accept( *this);

        // Getting condition result from evaluation stack
        ir::OperandPtr cmp_result = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        // Saving basic basic block which will go after if
        ir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( std::move( cmp_result), true_label, false_label));
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
        // FIXME store ast::CompareOp in node, it can prevent from many errors
        node.condition.get()->Accept( *this);

        ir::OperandPtr cmp_result = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        ir::LocalLabelID true_label = basic_blocks_counter_ + 1; // +1 is for next basic block, which will be used for false label
        ir::LocalLabelID false_label = basic_blocks_counter_ + 2;
        basic_blocks_counter_ += 2; // Saving two basic blocks which will not be used in body

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( std::move( cmp_result), true_label, false_label));
        finish_basic_block( true_label);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        basic_block_.instructions.emplace_back( std::make_unique<ir::CmpAndJmpInstr>( nullptr, condition_block, 0));
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

        ir::VarID result_tmp_id = tmp_counter_++;

        basic_block_.instructions.emplace_back( std::make_unique<ir::FunctionCallInstr>( result_tmp_id, node.id, std::move( parameters)));
    }

    void
    Visit( ast::Return& node) override
    {
        // Emitting expression to return
        node.expression.get()->Accept( *this);

        ir::OperandPtr expression = std::move( eval_stack_.back());
        eval_stack_.pop_back();

        basic_block_.instructions.emplace_back( std::make_unique<ir::UnaryOpInstr>( 0 /* unused */, ir::UnaryOpType::RET, std::move( expression)));
        finish_basic_block();
    }

    void
    Visit( ast::NewVariable& node) override
    {
        // FIXME
        // Getting new variable id using stack
        // It is better to save variable id in NewVariable node, as only it can occur there
        node.identifier.get()->Accept( *this);
        const ir::VarOperand *ident = static_cast<const ir::VarOperand *>( eval_stack_.back().get());
        ir::VarID ident_id = ident->id;
        eval_stack_.pop_back();

        // Counting new variable in stack, adds instructions to basic blocks
        if ( node.initializer != nullptr )
        {
            node.initializer.get()->Accept( *this);
        } else
        {
            eval_stack_.push_back( std::make_unique<ir::ImmOperand>( 0));
        }

        // Adding initialization instruction, adding information about stack size to function
        // Note: first. function is always preamble
        function_.stack_size++;
        ir::OperandPtr initializer = std::move( eval_stack_.back());
        eval_stack_.pop_back();
        basic_block_.instructions.push_back( std::make_unique<ir::UnaryOpInstr>( ident_id, ir::UnaryOpType::MOV, std::move( initializer)));
    }

    void
    Visit( ast::Function& node) override
    {
        start_function( node.id);

        // Getting function parameters
        for ( auto& it : node.parameters )
        {
            // FIXME Function node can also store parameters id's as only they can occur there
            it.get()->Accept( *this);
            const ir::VarOperand *ident = static_cast<const ir::VarOperand *>( eval_stack_.back().get());
            ir::VarID ident_id = ident->id;
            eval_stack_.pop_back();
            function_.params.emplace_back( ident_id);
        }

        // Emitting function body
        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        // TODO check if needed
        // Adding last basic block which can be not full to function
        finish_basic_block();
        // This is obviously needed
        finish_function();
    }

    void
    Visit( ast::Program& node) override
    {
        tmp_counter_ = node.nametable.GetMaxSymbolIndex();

        start_function( 0);

        // Global variables preamble
        for ( auto& it : node.global_variables )
        {
            it.get()->Accept( *this);
        }

        finish_basic_block(); // TODO check if needed
        finish_function();

        // Functions
        for ( auto& it : node.functions )
        {
            it.get()->Accept( *this);
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
EmitIR( ast::ASTNodePtr program)
{
    Emitter emitter{};

    program.get()->Accept( emitter);

    ir::Program program_ir = emitter.GetProgram();
    return program_ir;
}

} // ! namespace emit_ir
} // ! namespace dumb
