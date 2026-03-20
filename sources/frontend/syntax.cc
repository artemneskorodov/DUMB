#include <cassert>
#include <iostream>
#include <sstream>

#include "syntax.hh"
#include "nametable.hh"

namespace dumb
{
namespace syntax
{

namespace
{

class SyntaxError : public std::runtime_error
{
public:
    SyntaxError( int                line,
                 int                column,
                 const std::string &explanation)
     :  std::runtime_error { explanation},
        line_              { line},
        column_            { column}
    {
    }

    int Line()   const { return line_; }
    int Column() const { return column_; }

private:
    int line_;
    int column_;

};

class SyntaxParser final
{
public:
    SyntaxParser( const std::vector<lexer::Token> &tokens)
     :  tokens_{ tokens}
    {
    }

    ast::Program GetProgram();

private:
    ast::Function               get_function();
    std::list<ast::StmtNodePtr> get_body();
    ast::StmtNodePtr            get_statement();
    ast::StmtNodePtr            get_if();
    ast::StmtNodePtr            get_while();
    ast::StmtNodePtr            get_assignment();
    ast::StmtNodePtr            get_return();
    ast::StmtNodePtr            get_new_var();
    ast::StmtNodePtr            get_input();
    ast::StmtNodePtr            get_output();
    ast::CompareOp              get_comparison();
    ast::ExprNodePtr            get_expression();
    ast::ExprNodePtr            get_product();
    ast::ExprNodePtr            get_element();
    ast::ExprNodePtr            get_immediate();
    ast::ExprNodePtr            get_symbol();

private:
    inline void
    check( lexer::TokenType type,
           std::string      error = "",
           int              offset = 0)
    {
        if ( !is_type( type, offset) )
        {
            if ( error.empty() )
            {
                error = "Expected: " + lexer::TypeToStr( type) + " token, have: " +
                        lexer::TypeToStr( token().type);
            }
            syntax_error( error);
        }
    }

    inline void
    syntax_error( std::string error)
    {
        throw SyntaxError{ line(), column(), std::move( error)};
    }

    inline bool
    is_type( lexer::TokenType type,
             int              offset = 0) const
    {
        return (token( offset).type == type);
    }

    inline int
    line( int offset = 0) const
    {
        return token( offset).line;
    }

    inline int
    column( int offset = 0) const
    {
        return token( offset).column;
    }

    inline const std::string&
    value( int offset = 0) const
    {
        return token( offset).value;
    }

    inline const lexer::Token&
    token( int offset = 0) const
    {
        std::cerr << "On token [" << std::to_string( tokens_[pos_ + offset].line) << ":"
                  << std::to_string( tokens_[pos_ + offset].column) << "] "
                  << "type = " << lexer::TypeToStr( tokens_[pos_ + offset].type)
                  << ", value = '" << tokens_[pos_ + offset].value << "'\n";
        return tokens_[pos_ + offset];
    }

    inline void advance( int count = 1)
    {
        pos_ += count;
    }

private:
    const std::vector<lexer::Token> &tokens_;
    size_t pos_{ 0};
    nametable::NameTable nametable_{};

};

ast::Program
SyntaxParser::GetProgram()
{
    std::list<ast::Function> functions;
    std::list<ast::StmtNodePtr> variables;
    while ( !is_type( lexer::TokenType::END_OF_PROGRAM) )
    {
        if ( is_type( lexer::TokenType::FUNC_DECLARATION) )
        {
            functions.emplace_back( get_function());
        } else if ( is_type( lexer::TokenType::VAR_DEClARATION) )
        {
            variables.emplace_back( get_new_var());
        } else
        {
            syntax_error( "Unexpected global token");
        }
    }
    return ast::Program{ std::move( functions),
                         std::move( variables),
                         std::move( nametable_)};
}

ast::Function
SyntaxParser::get_function()
{
    assert( !nametable_.HasScope());

    check( lexer::TokenType::FUNC_DECLARATION);
    advance();

    check( lexer::TokenType::USER_STRING);
    std::string name = value();
    std::size_t id = nametable_.AddSymbol( name, nametable::Symbol::Type::FUNCTION);
    advance();

    check( lexer::TokenType::LEFT_PARENTHESIS);
    advance();

    nametable_.EnterScope();
    std::list<ir::VarID> parameters{};
    while ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
    {
        check( lexer::TokenType::USER_STRING);
        std::string name = value();
        ir::VarID param_id = nametable_.AddSymbol( name, nametable::Symbol::Type::LOCAL_VARIABLE);
        advance();

        parameters.emplace_back( param_id);

        if ( is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
        {
            break;
        } else if ( is_type( lexer::TokenType::COMMA) )
        {
            advance();
            continue;
        }
        syntax_error( "Expected COMMA");
    }
    advance();

    std::list<ast::StmtNodePtr> body = get_body();
    nametable_.LeaveScope();
    return ast::Function{ id, std::move( parameters), std::move( body)};
}

std::list<ast::StmtNodePtr>
SyntaxParser::get_body()
{
    check( lexer::TokenType::LEFT_SCOPE);
    advance();

    std::list<ast::StmtNodePtr> statements;

    while ( !is_type( lexer::TokenType::RIGHT_SCOPE) )
    {
        statements.emplace_back( get_statement());
    }
    advance();

    return statements;
};

ast::StmtNodePtr
SyntaxParser::get_statement()
{
    if ( is_type( lexer::TokenType::IF_STATEMENT) )
    {
        return get_if();
    } else if ( is_type(lexer::TokenType::WHILE_STATEMENT) )
    {
        return get_while();
    } else if ( is_type( lexer::TokenType::USER_STRING, 0) &&
                is_type( lexer::TokenType::ASSIGNMENT, 1) )
    {
        return get_assignment();
    } else if ( is_type( lexer::TokenType::RETURN_STATEMENT) )
    {
        return get_return();
    } else if ( is_type( lexer::TokenType::VAR_DEClARATION) )
    {
        return get_new_var();
    } else if ( is_type( lexer::TokenType::INPUT_STATEMENT) )
    {
        return get_input();
    } else if ( is_type( lexer::TokenType::OUTPUT_STATEMENT) )
    {
        return get_output();
    } else
    {
        syntax_error( "Expected statement");
        return nullptr;
    }
}

ast::StmtNodePtr
SyntaxParser::get_new_var()
{
    assert( is_type( lexer::TokenType::VAR_DEClARATION));
    advance();

    check( lexer::TokenType::USER_STRING);
    // Adding to nametable
    std::string name = value();
    ir::VarID id;
    if ( nametable_.HasScope() )
    {
        id = nametable_.AddSymbol( name, nametable::Symbol::Type::LOCAL_VARIABLE);
    } else
    {
        id = nametable_.AddSymbol( name, nametable::Symbol::Type::GLOBAL_VARIABLE);
    }
    advance();


    ast::ExprNodePtr initializer = nullptr;
    if ( is_type( lexer::TokenType::ASSIGNMENT) )
    {
        advance();
        initializer = get_expression();
    }

    ast::StmtNodePtr new_variable = std::make_unique<ast::NewVariable>( id,
                                                                        std::move( initializer));

    check( lexer::TokenType::STATEMENT_END);
    advance();
    return new_variable;
}

ast::StmtNodePtr
SyntaxParser::get_if()
{
    assert( is_type( lexer::TokenType::IF_STATEMENT));
    advance();

    check( lexer::TokenType::LEFT_PARENTHESIS);
    advance();

    ast::CompareOp condition = get_comparison();

    check( lexer::TokenType::RIGHT_PARENTHESIS);
    advance();

    nametable_.EnterScope();
    std::list<ast::StmtNodePtr> body = get_body();
    nametable_.LeaveScope();

    return std::make_unique<ast::If>( std::move( condition), std::move( body));
}

ast::StmtNodePtr
SyntaxParser::get_while()
{
    assert( is_type( lexer::TokenType::WHILE_STATEMENT));
    advance();

    check( lexer::TokenType::LEFT_PARENTHESIS);
    advance();

    ast::CompareOp condition = get_comparison();

    check( lexer::TokenType::RIGHT_PARENTHESIS);
    advance();

    nametable_.EnterScope();
    std::list<ast::StmtNodePtr> body = get_body();
    nametable_.LeaveScope();

    return std::make_unique<ast::While>( std::move( condition), std::move( body));
}

ast::StmtNodePtr
SyntaxParser::get_assignment()
{
    assert( is_type( lexer::TokenType::USER_STRING));
    assert( is_type( lexer::TokenType::ASSIGNMENT, 1));

    std::string name = value();
    auto sym = nametable_.GetSymbol( name);
    if ( !sym.has_value() )
    {
        syntax_error( "Unknown symbol");
    }
    if ( sym.value().GetType() == nametable::Symbol::Type::FUNCTION )
    {
        syntax_error( sym.value().GetName() + " is a function");
    }

    advance( 2); // Skipping 'user-string' and '='

    ast::ExprNodePtr expression = get_expression();

    check( lexer::TokenType::STATEMENT_END);
    advance();

    return std::make_unique<ast::Assignment>( sym.value().GetID(), std::move( expression));
}

ast::StmtNodePtr
SyntaxParser::get_return()
{
    assert( is_type( lexer::TokenType::RETURN_STATEMENT));
    advance();

    ast::ExprNodePtr expression = get_expression();

    check( lexer::TokenType::STATEMENT_END);
    advance();

    return std::make_unique<ast::Return>( std::move( expression));
}

ast::StmtNodePtr
SyntaxParser::get_input()
{
    assert( is_type( lexer::TokenType::INPUT_STATEMENT));
    advance();

    check( lexer::TokenType::LEFT_PARENTHESIS);
    advance();

    check( lexer::TokenType::USER_STRING);
    std::string name = value();
    auto sym = nametable_.GetSymbol( name);
    if ( !sym.has_value() )
    {
        syntax_error( "Unknown symbol");
    }
    if ( sym.value().GetType() == nametable::Symbol::Type::FUNCTION )
    {
        syntax_error( sym.value().GetName() + " is a function");
    }
    advance();

    check( lexer::TokenType::COMMA);
    advance();

    check( lexer::TokenType::USER_QUOTED_STRING);
    std::string string = value();
    advance();

    check( lexer::TokenType::RIGHT_PARENTHESIS);
    advance();

    check( lexer::TokenType::STATEMENT_END);
    advance();

    return std::make_unique<ast::Input>( sym.value().GetID(), std::move( string));
}

ast::StmtNodePtr
SyntaxParser::get_output()
{
    assert( is_type( lexer::TokenType::INPUT_STATEMENT));
    advance();

    check( lexer::TokenType::LEFT_PARENTHESIS);
    advance();

    ast::ExprNodePtr expression = get_expression();

    check( lexer::TokenType::COMMA);
    advance();

    check( lexer::TokenType::USER_QUOTED_STRING);
    std::string string = value();
    advance();

    check( lexer::TokenType::RIGHT_PARENTHESIS);
    advance();

    check( lexer::TokenType::STATEMENT_END);
    advance();

    return std::make_unique<ast::Output>( std::move( expression), std::move( string));
}

ast::CompareOp
SyntaxParser::get_comparison()
{
    ast::ExprNodePtr cmp_left_side = get_expression();

    ast::CompareOp::Operation operation;
    if ( is_type( lexer::TokenType::CMP_LESS) )
    {
        operation = ast::CompareOp::OP_CMP_LESS;
    } else if ( is_type( lexer::TokenType::CMP_EQUAL) )
    {
        operation = ast::CompareOp::OP_CMP_EQUAL;
    } else if ( is_type( lexer::TokenType::CMP_BIGGER) )
    {
        operation = ast::CompareOp::OP_CMP_BIGGER;
    } else
    {
        syntax_error( "Expected comparison token");
        // Used to avoid uninitialized variable warning
        return ast::CompareOp{ ast::CompareOp::Operation::OP_CMP_BIGGER, nullptr, nullptr};
    }
    advance();

    ast::ExprNodePtr cmp_right_side = get_expression();

    return ast::CompareOp{ operation,
                           std::move( cmp_left_side),
                           std::move( cmp_right_side)};
}

ast::ExprNodePtr
SyntaxParser::get_expression()
{
    ast::ExprNodePtr left_side = get_product();
    while ( is_type( lexer::TokenType::OP_ADD) ||
            is_type( lexer::TokenType::OP_SUB) )
    {
        ast::BinaryOp::Operation operation;
        switch ( token().type )
        {
            case lexer::TokenType::OP_ADD: operation = ast::BinaryOp::OP_ADD; break;
            case lexer::TokenType::OP_SUB: operation = ast::BinaryOp::OP_SUB; break;
            default: assert( false);
        }
        advance();

        ast::ExprNodePtr right_side = get_product();
        left_side = std::make_unique<ast::BinaryOp>( operation,
                                                     std::move( left_side),
                                                     std::move( right_side));
    }
    return left_side;
}

ast::ExprNodePtr
SyntaxParser::get_product()
{
    ast::ExprNodePtr left_side = get_element();
    while ( is_type( lexer::TokenType::OP_MUL) ||
            is_type( lexer::TokenType::OP_DIV) )
    {
        ast::BinaryOp::Operation operation;
        switch ( token().type )
        {
            case lexer::TokenType::OP_MUL: operation = ast::BinaryOp::OP_MUL; break;
            case lexer::TokenType::OP_DIV: operation = ast::BinaryOp::OP_DIV; break;
            default: assert( false);
        }
        advance();

        ast::ExprNodePtr right_side = get_product();
        left_side = std::make_unique<ast::BinaryOp>( operation,
                                                     std::move( left_side),
                                                     std::move( right_side));
    }
    return left_side;
}

ast::ExprNodePtr
SyntaxParser::get_element()
{
    if ( is_type( lexer::TokenType::USER_STRING) )
    {
        return get_symbol();
    } else if ( is_type( lexer::TokenType::IMMEDIATE) )
    {
        return get_immediate();
    } else if ( is_type (lexer::TokenType::LEFT_PARENTHESIS) )
    {
        ast::ExprNodePtr expression = get_expression();
        if ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
        {
            throw SyntaxError{ line(), column(),
                              "Expected ')' after expression"};
        }
        return expression;
    }else
    {
        syntax_error( "Expected element expression part");
        return nullptr;
    }
}

ast::ExprNodePtr
SyntaxParser::get_symbol()
{
    std::string name = value();
    auto sym = nametable_.GetSymbol( name);
    if ( !sym.has_value() )
    {
        syntax_error( "Undeclared symbol: " + name);
    }
    advance();

    if ( sym.value().GetType() == nametable::Symbol::Type::FUNCTION )
    {
        check( lexer::TokenType::LEFT_PARENTHESIS);
        advance();

        std::list<ast::ExprNodePtr> parameters{};
        while ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
        {
            parameters.emplace_back( get_expression());
        }
        advance();

        return std::make_unique<ast::FunctionCall>( sym.value().GetID(), std::move( parameters));
    } else
    {
        return std::make_unique<ast::Identifier>( sym.value().GetID());
    }
}

ast::ExprNodePtr
SyntaxParser::get_immediate()
{
    assert( is_type( lexer::TokenType::IMMEDIATE));
    ir::ImmType immediate = std::atoi( value().c_str());
    advance();

    return std::make_unique<ast::Immediate>( immediate);
}

} // ! anonymous namespace

ast::Program
ParseSyntax( const std::vector<lexer::Token>& tokens,
             const std::string&               filename)
{
    SyntaxParser parser{ tokens};
    try
    {
        return parser.GetProgram();
    } catch ( SyntaxError error )
    {
        std::cout << "\033[1;3;5;38;5;161;49m" << "Syntax error" << "\033[0m " << filename << ":"
                  << error.Line() << ":" << error.Column() << " " << error.what()
                  << std::endl;
        throw;
    }
}

} // ! namespace syntax
} // ! namespace dumb
