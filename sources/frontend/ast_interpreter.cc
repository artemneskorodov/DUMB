#include <unordered_map>
#include <vector>
#include <iostream>

#include "ast.hh"
#include "ast_interpreter.hh"

namespace dumb
{
namespace ast
{
namespace interpreter
{

namespace
{

class Interpreter : public Visitor
{
private:
    void
    Visit( Immediate& node) override
    {
        eval_stack_.push_back( node.value);
    }

    void
    Visit( Identifier& node) override
    {
        eval_stack_.push_back(values_[node.id]);
    }

    void
    Visit( BinaryOp& node) override
    {
        node.left->Accept( *this);
        int left = eval_stack_.back();
        eval_stack_.pop_back();
        node.right->Accept( *this);
        int right = eval_stack_.back();
        eval_stack_.pop_back();

        int result;
        switch ( node.operation )
        {
            case BinaryOp::OP_ADD: result = left + right; break;
            case BinaryOp::OP_SUB: result = left - right; break;
            case BinaryOp::OP_MUL: result = left * right; break;
            case BinaryOp::OP_DIV: result = left / right; break;
            default: throw std::runtime_error{ "Unexpected binary operation"};
        }
        eval_stack_.push_back( result);
    }

    void
    Visit( Assignment& node) override
    {
        node.right->Accept( *this);
        int value = eval_stack_.back();
        eval_stack_.pop_back();
        values_[node.left] = value;
    }

    void
    Visit( If& node) override
    {
        node.condition.left->Accept( *this);
        int left = eval_stack_.back();
        eval_stack_.pop_back();
        node.condition.right->Accept( *this);
        int right = eval_stack_.back();
        eval_stack_.pop_back();

        bool result;
        switch ( node.condition.operation )
        {
            case CompareOp::OP_CMP_LESS:    result = (left <  right); break;
            case CompareOp::OP_CMP_EQUAL:   result = (left == right); break;
            case CompareOp::OP_CMP_BIGGER:  result = (left >  right); break;
            default: throw std::runtime_error{ "Unexpected compare operation"};
        }
        if ( !result )
        {
            return ;
        }
        for ( auto& stmt : node.body )
        {
            stmt->Accept( *this);
            if ( need_return_ )
            {
                return ;
            }
        }
    }

    void
    Visit( While& node) override
    {
        for ( ; ; )
        {
            node.condition.left->Accept( *this);
            int left = eval_stack_.back();
            eval_stack_.pop_back();
            node.condition.right->Accept( *this);
            int right = eval_stack_.back();
            eval_stack_.pop_back();

            bool result;
            switch ( node.condition.operation )
            {
                case CompareOp::OP_CMP_LESS:    result = (left <  right); break;
                case CompareOp::OP_CMP_EQUAL:   result = (left == right); break;
                case CompareOp::OP_CMP_BIGGER:  result = (left >  right); break;
                default: throw std::runtime_error{ "Unexpected compare operation"};
            }
            if ( !result )
            {
                break;
            }
            for ( auto& stmt : node.body )
            {
                stmt->Accept( *this);
                if ( need_return_ )
                {
                    return ;
                }
            }
        }
    }

    void
    Visit( FunctionCall& node) override
    {
        Function &func = find_function( node.id);
        for ( auto& param : node.parameters )
        {
            param->Accept( *this);
            parameters_.emplace_back( eval_stack_.back());
            eval_stack_.pop_back();
        }
        VisitFunction( func);
        need_return_ = false;
        eval_stack_.emplace_back( function_result_);
    }

    void
    Visit( Return& node) override
    {
        node.expression->Accept( *this);
        function_result_ = eval_stack_.back();
        eval_stack_.pop_back();
        need_return_ = true;
    }

    void
    Visit( NewVariable& node) override
    {
        if ( node.initializer != nullptr )
        {
            node.initializer->Accept( *this);
            values_[node.identifier] = eval_stack_.back();
            eval_stack_.pop_back();
        } else
        {
            values_[node.identifier] = 123123;
        }
    }

    void
    Visit( Input& node) override
    {
        std::cout << node.string;
        int value;
        std::cin >> value;
        values_[node.identifier] = value;
    }

    void
    Visit( Output& node) override
    {
        node.expression->Accept( *this);
        int value = eval_stack_.back();
        eval_stack_.pop_back();

        std::cout << node.string << value << std::endl;
    }

    void
    VisitFunction( Function& function)
    {
        for ( auto param_it = function.parameters.rbegin();
              param_it != function.parameters.rend();
              ++param_it )
        {
            values_[*param_it] = parameters_.back();
            parameters_.pop_back();
        }
        if ( !parameters_.empty() )
        {
            throw std::runtime_error{ "Parameters storage must be empty after poping in function"};
        }
        for ( auto& stmt : function.body )
        {
            stmt->Accept( *this);
            if ( need_return_ )
            {
                return ;
            }
        }
    }

public:
    void
    Run( ast::Program &ast)
    {
        program_ = &ast;
        Function& func = find_function( "main");
        VisitFunction( func);
    }

private:
    Function&
    find_function( nt::SymbolID id)
    {
        for ( Function& func : program_->functions )
        {
            if ( id == func.id )
            {
                return func;
            }
        }
        throw std::runtime_error{ "No function with id = " + std::to_string( id)};
    }

    Function&
    find_function( std::string name)
    {
        for ( Function& func : program_->functions )
        {
            const nt::Symbol *sym = program_->nametable.GetSymbol( func.id);
            if ( sym->GetName() == name )
            {
                return func;
            }
        }
        throw std::runtime_error{ "No function with name = \"" + name + "\""};
    }

private:
    ast::Program *program_;
    std::unordered_map<nt::SymbolID, int> values_{};
    int function_result_;
    std::vector<int> eval_stack_{};
    std::vector<int> parameters_{};
    bool need_return_{ false};

};

} // ! anonymous namespace

void
Run( ast::Program *ast)
{
    Interpreter interpreter{};
    interpreter.Run( *ast);
}

} // ! namespace interpreter
} // ! namespace ast
} // ! namespace dumb
