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
public:
    Emitter( ir::Program& program,
             ir::Function& function)
     :  program_{ program},
        function_{ function},
        basic_block_{ &function_.AddBasicBlock( basic_blocks_counter_++)}
    {
    }

    Emitter( ir::Program& program,
             ir::Function& function,
             ir::BasicBlock& basic_block)
     :  program_{ program},
        function_{ function},
        basic_block_{ &basic_block}
    {
    }

    void
    Visit( const ast::Immediate& node) override
    {
        eval_result_ = ir::Operand{ ir::Operand::IMMEDIATE, node.value};
    }

    void
    Visit( const ast::Identifier& node) override
    {
        eval_result_ = get_operand( node.id);
        if ( eval_result_.type != ir::Operand::VARIABLE &&
             eval_result_.type != ir::Operand::GLOBAL )
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }
    }

    void
    Visit( const ast::BinaryOp& node) override
    {
        // Emitting left side of operation
        node.left->Accept( *this);
        ir::Operand left = eval_result_;

        // Emitting right side of operation
        node.right->Accept( *this);
        ir::Operand right = eval_result_;

        // Adding instruction
        ir::Opcode opcode;
        switch ( node.operation )
        {
            case ast::BinaryOp::OP_ADD: opcode = ir::Opcode::ADD; break;
            case ast::BinaryOp::OP_SUB: opcode = ir::Opcode::SUB; break;
            case ast::BinaryOp::OP_MUL: opcode = ir::Opcode::MUL; break;
            case ast::BinaryOp::OP_DIV: opcode = ir::Opcode::DIV; break;
            default: throw std::runtime_error{ "Unexpected binary operation type"};
        }

        eval_result_ = get_tmp();
        basic_block_->instructions.emplace_back( opcode,
                                                 eval_result_,
                                                 std::vector<ir::Operand>{
                                                     left,
                                                     right
                                                 });
    }

    void
    Visit( const ast::Assignment& node) override
    {
        // Emitting expression
        node.right->Accept( *this);

        ir::Operand dest = get_operand( node.left);

        basic_block_->instructions.emplace_back( ir::Opcode::MOV,
                                                 dest,
                                                 std::vector<ir::Operand>{
                                                     eval_result_
                                                 });
    }

    void
    Visit( const ast::If& node) override
    {
        // Left
        node.condition.left->Accept( *this);
        ir::Operand left = eval_result_;
        //Right
        node.condition.right->Accept( *this);
        ir::Operand right = eval_result_;

        ir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = ir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = ir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = ir::CmpType::BIGGER; break;
            default: throw std::runtime_error{ "Unexpected compare operation type"};
        }

        // Saving basic basic block which will go after if
        std::pair<int, int> labels = book_basic_blocks_pair();

        basic_block_->terminator = ir::BasicBlockTerminator( left,
                                                             right,
                                                             type,
                                                             labels.first,
                                                             labels.second);
        start_new_basic_block( labels.first);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        basic_block_->terminator.type      = ir::CmpType::ALWAYS_TRUE;
        basic_block_->terminator.true_dest = labels.second;

        // Finishing basic block with new basic block label equals to false label which we saved previously
        start_new_basic_block(labels.second);
    }

    void
    Visit( const ast::While& node) override
    {
        // Adding condition basic block
        basic_block_->terminator.type      = ir::CmpType::ALWAYS_TRUE;
        basic_block_->terminator.true_dest = current_basic_block() + 1;

        start_new_basic_block();
        int condition_block = current_basic_block();

        // Emitting condition
        // Left
        node.condition.left->Accept( *this);
        ir::Operand left = eval_result_;
        //Right
        node.condition.right->Accept( *this);
        ir::Operand right = eval_result_;

        ir::CmpType type;
        switch ( node.condition.operation )
        {
            case ast::CompareOp::Operation::OP_CMP_LESS:   type = ir::CmpType::LESS;   break;
            case ast::CompareOp::Operation::OP_CMP_EQUAL:  type = ir::CmpType::EQUAL;  break;
            case ast::CompareOp::Operation::OP_CMP_BIGGER: type = ir::CmpType::BIGGER; break;
            default: throw std::runtime_error{ "Unexpected compare operation type"};
        }

        std::pair<int, int> labels = book_basic_blocks_pair();

        basic_block_->terminator = ir::BasicBlockTerminator( left,
                                                             right,
                                                             type,
                                                             labels.first,
                                                             labels.second);

        start_new_basic_block( labels.first);

        for ( auto& it : node.body )
        {
            it.get()->Accept( *this);
        }

        basic_block_->terminator.type      = ir::CmpType::ALWAYS_TRUE;
        basic_block_->terminator.true_dest = condition_block;

        start_new_basic_block( labels.second);
    }

    void
    Visit( const ast::FunctionCall& node) override
    {
        std::vector<ir::Operand> operands{};
        operands.emplace_back( get_operand( node.id));
        if ( operands.back().type != ir::Operand::FUNC_LABEL )
        {
            throw std::runtime_error{ "Unexpected symbol type"};
        }

        for ( const ast::ExprNodePtr& param : node.parameters )
        {
            param->Accept( *this);
            operands.emplace_back( eval_result_);
        }

        eval_result_ = get_tmp();
        basic_block_->instructions.emplace_back( ir::Opcode::CALL,
                                                 eval_result_,
                                                 std::move( operands));
    }

    void
    Visit( const ast::Return& node) override
    {
        // Emitting expression to return
        node.expression.get()->Accept( *this);
        ir::Operand expression = eval_result_;

        basic_block_->instructions.emplace_back( ir::Opcode::RET,
                                                 ir::kNoDefine,
                                                 std::vector<ir::Operand>{ expression});
    }

    void
    Visit( const ast::NewVariable& node) override
    {
        ir::Operand dest = get_operand( node.identifier);

        if ( dest.type == ir::Operand::VARIABLE )
        {
            function_.AddVariable( dest.id, 0);
        } else if ( dest.type == ir::Operand::GLOBAL )
        {
            program_.AddGlobal( dest.id);
        } else
        {
            throw std::runtime_error{ "Unexpected dest type"};
        }

        if ( node.initializer != nullptr )
        {
            node.initializer->Accept( *this);
        } else
        {
            return;
        }
        ir::Operand initializer = eval_result_;

        basic_block_->instructions.emplace_back( ir::Opcode::MOV,
                                                 dest,
                                                 std::vector<ir::Operand>{
                                                     initializer
                                                 });
    }

    void
    Visit( const ast::Input& node) override
    {
        ir::Operand dest = get_operand( node.identifier);
        if ( dest.type != ir::Operand::VARIABLE &&
             dest.type != ir::Operand::GLOBAL )
        {
            throw std::runtime_error{ "Unexpected dest type"};
        }

        int id = program_.AddString( node.string);

        basic_block_->instructions.emplace_back( ir::Opcode::INPUT,
                                                 dest,
                                                 std::vector<ir::Operand>{
                                                     ir::Operand{ ir::Operand::STRING_LABEL, id}
                                                 });
    };

    void
    Visit( const ast::Output& node) override
    {
        node.expression->Accept( *this);
        ir::Operand expression = eval_result_;

        int id = program_.AddString( node.string);

        basic_block_->instructions.emplace_back( ir::Opcode::OUTPUT,
                                                 ir::kNoDefine,
                                                 std::vector<ir::Operand>{
                                                     { ir::Operand::STRING_LABEL, id},
                                                     expression
                                                 });
    }

private:
    ir::Operand               eval_result_{};
    std::size_t               tmp_counter_{ 0};
    int                       basic_blocks_counter_{ 0};

    ir::Program              &program_;
    ir::Function             &function_;
    ir::BasicBlock           *basic_block_;

private:
    void
    start_new_basic_block( int id)
    {
        basic_block_ = &function_.AddBasicBlock( id);
    }

    void
    start_new_basic_block()
    {
        basic_block_ = &function_.AddBasicBlock( basic_blocks_counter_);
        ++basic_blocks_counter_;
    }

    int
    current_basic_block() const
    {
        return basic_blocks_counter_ - 1;
    }

    std::pair<int, int>
    book_basic_blocks_pair()
    {
        basic_blocks_counter_ += 2;
        return { basic_blocks_counter_ - 2, basic_blocks_counter_ - 1};
    }

    ir::Operand
    get_operand( nt::SymbolID id)
    {
        const nt::Symbol *sym = program_.Nametable().FindSymbol( id);

        if ( sym->GetType() == nt::SymbolType::LOCAL_VARIABLE )
        {
            return ir::Operand{ ir::Operand::VARIABLE, 0, id};
        } else if ( sym->GetType() == nt::SymbolType::GLOBAL_VARIABLE )
        {
            return ir::Operand{ ir::Operand::GLOBAL, 0, id};
        } else if ( sym->GetType() == nt::SymbolType::FUNCTION )
        {
            return ir::Operand{ ir::Operand::FUNC_LABEL, 0, id};
        } else
        {
            throw std::runtime_error{ "Unexpected operand type"};
        }
    }

    ir::Operand
    get_tmp()
    {
        std::string tmp_name = "__tmp_" + std::to_string( ++tmp_counter_);
        nt::SymbolID tmp_id = program_.Nametable().AddSymbol( tmp_name,
                                                              nt::SymbolType::LOCAL_VARIABLE);
        function_.AddVariable( tmp_id, 0);

        return ir::Operand{ ir::Operand::VARIABLE, 0, tmp_id};
    }
};

} // ! anonymous namespace

ir::Program
EmitIR( const ast::Program& program)
{
    ir::Program ir{ program.nametable};

    nt::SymbolID start_id = ir.Nametable().AddSymbol( "_start", nt::SymbolType::FUNCTION);

    ir::Function& start = ir.Preamble( start_id);
    ir::BasicBlock& basic_block = start.AddBasicBlock( 0);

    Emitter emitter{ ir, start, basic_block};
    for ( const ast::StmtNodePtr& stmt : program.global_variables )
    {
        stmt->Accept( emitter);
    }

    const nt::Symbol *main_sym = ir.Nametable().FindSymbol( "main");

    basic_block.instructions.emplace_back( ir::Opcode::CALL,
                                           ir::kNoDefine,
                                           std::vector<ir::Operand>{
                                               { ir::Operand::FUNC_LABEL, 0, main_sym->GetID()}
                                           });

    for ( const ast::Function& func : program.functions )
    {
        ir::Function& function_ir = ir.AddFunction( func.id);

        Emitter emitter{ ir, function_ir};
        for ( nt::SymbolID param : func.parameters )
        {
            function_ir.AddParam( param, 0);
        }

        for ( const auto& stmt : func.body )
        {
            stmt->Accept( emitter);
        }
    }

    return ir;
}

} // ! namespace emit_ir
} // ! namespace dumb
