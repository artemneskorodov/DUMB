#include <cassert>
#include <iostream>

#include "syntax.hh"

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

    ast::ASTNodePtr GetProgram();

private:
    ast::ASTNodePtr get_function();
    std::list<ast::ASTNodePtr> get_body();
    ast::ASTNodePtr get_statement();
    ast::ASTNodePtr get_if();
    ast::ASTNodePtr get_while();
    ast::ASTNodePtr get_assignment();
    ast::ASTNodePtr get_return();
    ast::ASTNodePtr get_expression();
    ast::ASTNodePtr get_sum();
    ast::ASTNodePtr get_product();
    ast::ASTNodePtr get_element();
    ast::ASTNodePtr get_function_call();
    ast::ASTNodePtr get_immediate();
    ast::ASTNodePtr get_identifier();
    ast::ASTNodePtr get_new_var();

private:
    inline bool is_type( lexer::TokenType type, int offset = 0) const { return (token( offset).type == type); }
    inline int line( int offset = 0) const { return token( offset).line; }
    inline int column( int offset = 0) const { return token( offset).column; }
    inline const std::string& value( int offset = 0) const { return token( offset).value; }
    inline const lexer::Token& token( int offset = 0) const { return tokens_[pos_ + offset]; }
    inline void advance( int count = 1) { pos_ += count; }

private:
    const std::vector<lexer::Token> &tokens_;
    size_t pos_ = 0;

};

ast::ASTNodePtr
SyntaxParser::GetProgram()
{
    std::list<ast::ASTNodePtr> functions;
    while ( !is_type( lexer::TokenType::END_OF_PROGRAM) )
    {
        functions.emplace_back( get_function());
    }
    return std::make_unique<ast::Program>( std::move( functions));
}

ast::ASTNodePtr
SyntaxParser::get_function()
{
    if ( !is_type( lexer::TokenType::FUNC_DECLARATION) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected functions declaration"};
    }
    advance();

    if ( !is_type( lexer::TokenType::USER_STRING) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected function name"};
    }
    std::string name = value();
    advance();

    if ( !is_type( lexer::TokenType::LEFT_PARENTHESIS) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected '(' and parameters list after function declaration"};
    }
    advance();

    std::list<ast::ASTNodePtr> parameters{};

    while ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
    {
        parameters.emplace_back( get_identifier());
    }
    advance();

    std::list<ast::ASTNodePtr> body = get_body();
    return std::make_unique<ast::Function>( name, std::move( parameters), std::move( body));
}

std::list<ast::ASTNodePtr>
SyntaxParser::get_body()
{
    if ( !is_type( lexer::TokenType::LEFT_SCOPE) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected body start: '{'"};
    }
    advance();

    std::list<ast::ASTNodePtr> statements;

    while ( !is_type( lexer::TokenType::RIGHT_SCOPE) )
    {
        statements.emplace_back( get_statement());
    }
    advance();

    return statements;
};

ast::ASTNodePtr
SyntaxParser::get_identifier()
{
    if ( !is_type( lexer::TokenType::USER_STRING) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected identifier"};
    }
    std::string name = value();
    advance();

    return std::make_unique<ast::Identifier>( name);
}

ast::ASTNodePtr
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
        ast::ASTNodePtr result = get_assignment();
        if ( !is_type( lexer::TokenType::STATEMENT_END) )
        {
            throw SyntaxError{ line(), column(),
                              "Expected ';'"};
        }
        advance();
        return result;
    } else if ( is_type( lexer::TokenType::RETURN_STATEMENT) )
    {
        ast::ASTNodePtr result = get_return();
        if ( !is_type( lexer::TokenType::STATEMENT_END) )
        {
            throw SyntaxError{ line(), column(),
                              "Expected ';'"};
        }
        advance();
        return result;
    } else if ( is_type( lexer::TokenType::VAR_DEClARATION) )
    {
        ast::ASTNodePtr result = get_new_var();
        if ( !is_type( lexer::TokenType::STATEMENT_END) )
        {
            throw SyntaxError{ line(), column(),
                              "Expected ';'"};
        }
        advance();
        return result;
    }else
    {
        throw SyntaxError{ line(), column(),
                          "Expected statement"};
    }
}

ast::ASTNodePtr
SyntaxParser::get_new_var()
{
    assert( is_type( lexer::TokenType::VAR_DEClARATION));
    advance();

    ast::ASTNodePtr identifier = get_identifier();

    if ( !is_type( lexer::TokenType::ASSIGNMENT) )
    {
        return std::make_unique<ast::NewVariable>( std::move( identifier), nullptr);
    } else
    {
        advance();
        ast::ASTNodePtr initializer = get_sum();
        return std::make_unique<ast::NewVariable>( std::move( identifier), std::move( initializer));
    }
}

ast::ASTNodePtr
SyntaxParser::get_if()
{
    assert( is_type( lexer::TokenType::IF_STATEMENT));
    advance();

    if ( !is_type( lexer::TokenType::LEFT_PARENTHESIS) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected condition after if"};
    }
    advance();

    ast::ASTNodePtr condition = get_expression();

    if ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected end of condition"};
    }
    advance();

    std::list<ast::ASTNodePtr> body = get_body();
    return std::make_unique<ast::If>( std::move( condition), std::move( body));
}

ast::ASTNodePtr
SyntaxParser::get_while()
{
    assert( is_type( lexer::TokenType::WHILE_STATEMENT));
    advance();

    if ( !is_type( lexer::TokenType::LEFT_PARENTHESIS) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected condition after while"};
    }
    advance();

    ast::ASTNodePtr condition = get_expression();

    if ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
    {
        throw SyntaxError{ line(), column(),
                          "Expected end of condition"};
    }
    advance();

    std::list<ast::ASTNodePtr> body = get_body();

    return std::make_unique<ast::While>( std::move( condition), std::move( body));
}

ast::ASTNodePtr
SyntaxParser::get_assignment()
{
    assert( is_type( lexer::TokenType::USER_STRING));
    assert( is_type( lexer::TokenType::ASSIGNMENT, 1));

    ast::ASTNodePtr left = get_identifier();

    advance();

    ast::ASTNodePtr expression = get_expression();

    return std::make_unique<ast::Assignment>( std::move( left), std::move( expression));
}

ast::ASTNodePtr
SyntaxParser::get_return()
{
    assert( is_type( lexer::TokenType::RETURN_STATEMENT));
    advance();

    ast::ASTNodePtr expression = get_expression();

    return std::make_unique<ast::Return>( std::move( expression));
}

ast::ASTNodePtr
SyntaxParser::get_expression()
{
    ast::ASTNodePtr cmp_left_side = get_sum();
    if ( is_type( lexer::TokenType::CMP_LESS) ||
         is_type( lexer::TokenType::CMP_EQUAL) ||
         is_type( lexer::TokenType::CMP_BIGGER) )
    {
        ast::BinaryOp::Operation operation;
        switch ( token().type )
        {
            case lexer::TokenType::CMP_LESS:   operation = ast::BinaryOp::OP_CMP_LESS;   break;
            case lexer::TokenType::CMP_EQUAL:  operation = ast::BinaryOp::OP_CMP_EQUAL;  break;
            case lexer::TokenType::CMP_BIGGER: operation = ast::BinaryOp::OP_CMP_BIGGER; break;
            default: assert( false);
        }
        advance();

        ast::ASTNodePtr cmp_right_side = get_sum();
        return std::make_unique<ast::BinaryOp>( operation,
                                                std::move( cmp_left_side),
                                                std::move( cmp_right_side));
    } else
    {
        return cmp_left_side;
    }
}

ast::ASTNodePtr
SyntaxParser::get_sum()
{
    ast::ASTNodePtr left_side = get_product();
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

        ast::ASTNodePtr right_side = get_product();
        left_side = std::make_unique<ast::BinaryOp>( operation,
                                                     std::move( left_side),
                                                     std::move( right_side));
    }
    return left_side;
}

ast::ASTNodePtr
SyntaxParser::get_product()
{
    ast::ASTNodePtr left_side = get_element();
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

        ast::ASTNodePtr right_side = get_product();
        left_side = std::make_unique<ast::BinaryOp>( operation,
                                                     std::move( left_side),
                                                     std::move( right_side));
    }
    return left_side;
}

ast::ASTNodePtr
SyntaxParser::get_element()
{
    if ( is_type( lexer::TokenType::USER_STRING) )
    {
        if ( is_type( lexer::TokenType::LEFT_PARENTHESIS, 1 ) )
        {
            return get_function_call();
        } else
        {
            return get_identifier();
        }
    } else if ( is_type( lexer::TokenType::IMMEDIATE) )
    {
        return get_immediate();
    } else if ( is_type (lexer::TokenType::LEFT_PARENTHESIS) )
    {
        ast::ASTNodePtr expression = get_sum();
        if ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
        {
            throw SyntaxError{ line(), column(),
                              "Expected ')' after expression"};
        }
        return expression;
    }else
    {
        throw SyntaxError{ line(), column(),
                          "Expected element expression part: identifier, immediate or function call"};
    }
}

ast::ASTNodePtr
SyntaxParser::get_function_call()
{
    assert( is_type( lexer::TokenType::USER_STRING));
    assert( is_type( lexer::TokenType::LEFT_PARENTHESIS, 1));

    std::string name = value();
    advance( 2);

    std::list<ast::ASTNodePtr> parameters{};
    while ( !is_type( lexer::TokenType::RIGHT_PARENTHESIS) )
    {
        parameters.emplace_back( get_expression());
    }
    return std::make_unique<ast::FunctionCall>( name, std::move( parameters));
}

ast::ASTNodePtr
SyntaxParser::get_immediate()
{
    assert( is_type( lexer::TokenType::IMMEDIATE));
    int immediate = std::atoi( value().c_str());
    advance();
    return std::make_unique<ast::Immediate>( immediate);
}

} // ! anonymous namespace

ast::ASTNodePtr
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
        return nullptr;
    }
}

} // ! namespace syntax
} // ! namespace dumb
