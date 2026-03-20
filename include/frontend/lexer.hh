#ifndef DUMB_LEXER_HH__
#define DUMB_LEXER_HH__

#include <vector>
#include <string>
#include <optional>
#include <cstdint>

namespace dumb
{
namespace lexer
{

///
/// @brief
///
enum class TokenType
{
    USER_STRING,
    IMMEDIATE,
    FUNC_DECLARATION,
    VAR_DEClARATION,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    LEFT_SCOPE,
    RIGHT_SCOPE,
    IF_STATEMENT,
    WHILE_STATEMENT,
    RETURN_STATEMENT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    ASSIGNMENT,
    CMP_LESS,
    CMP_BIGGER,
    CMP_EQUAL,
    STATEMENT_END,
    END_OF_PROGRAM,
    COMMA,
    INPUT_STATEMENT,
    OUTPUT_STATEMENT,
    USER_QUOTED_STRING,
};

std::string
TypeToStr( TokenType type)
{
    switch ( type )
    {
        case TokenType::USER_STRING:        return "USER_STRING";
        case TokenType::IMMEDIATE:          return "IMMEDIATE";
        case TokenType::FUNC_DECLARATION:   return "FUNC_DECLARATION";
        case TokenType::VAR_DEClARATION:    return "VAR_DECLARATION";
        case TokenType::LEFT_PARENTHESIS:   return "LEFT_PARENTHESIS";
        case TokenType::RIGHT_PARENTHESIS:  return "RIGHT_PARENTHESIS";
        case TokenType::LEFT_SCOPE:         return "LEFT_SCOPE";
        case TokenType::RIGHT_SCOPE:        return "RIGHT_SCOPE";
        case TokenType::IF_STATEMENT:       return "IF_STATEMENT";
        case TokenType::WHILE_STATEMENT:    return "WHILE_STATEMENT";
        case TokenType::RETURN_STATEMENT:   return "RETURN_STATEMENT";
        case TokenType::OP_ADD:             return "OP_ADD";
        case TokenType::OP_SUB:             return "OP_SUB";
        case TokenType::OP_MUL:             return "OP_MUL";
        case TokenType::OP_DIV:             return "OP_DIV";
        case TokenType::ASSIGNMENT:         return "ASSIGNMENT";
        case TokenType::CMP_LESS:           return "CMP_LESS";
        case TokenType::CMP_BIGGER:         return "CMP_BIGGER";
        case TokenType::CMP_EQUAL:          return "CMP_EQUAL";
        case TokenType::STATEMENT_END:      return "STATEMENT_END";
        case TokenType::END_OF_PROGRAM:     return "END_OF_PROGRAM";
        case TokenType::COMMA:              return "COMMA";
        case TokenType::INPUT_STATEMENT:    return "INPUT_STATEMENT";
        case TokenType::OUTPUT_STATEMENT:   return "OUTPUT_STATEMENT";
        case TokenType::USER_QUOTED_STRING: return "USER_QUOTED_STRING";
    }
}

template<typename TOutStream>
TOutStream&
operator<<( TOutStream& os, TokenType token_type)
{
    return (os << TypeToStr( token_type));
}

///
/// @brief
///
struct Token
{
    Token( TokenType   type,
           std::string value,
           int         line,
           int         column)
     :  type   { type},
        value  { std::move( value)},
        line   { line},
        column { column}
    {
    }

    TokenType   type;
    std::string value;
    int         line;
    int         column;

};

///
/// @brief
///
class Lexer final
{
public:
    explicit Lexer( const std::string &source);

public:
    std::vector<Token> Tokenize();

private:
    std::string        source_;
    std::vector<Token> tokens_;
    size_t             pos_    {0};
    int                line_   {1};
    int                column_ {1};

private:
    void                 advance            ();
    void                 skip_spaces        ();
    char                 current_char       ( int offset = 0) const;
    Token                read_immediate     ();
    Token                read_word          ();
    std::optional<Token> read_symbol        ();
    Token                read_quoted_string ();

};

} // ! namespace lexer
} // ! namespace dumb

#endif // ! DUMB_LEXER_HH__
