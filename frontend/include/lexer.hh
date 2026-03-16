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
};

template<typename TOutStream>
TOutStream&
operator<<( TOutStream& os, TokenType token_type)
{
    std::string type_string;
    switch ( token_type )
    {
        case TokenType::USER_STRING:        type_string = "USER_STRING";        break;
        case TokenType::IMMEDIATE:          type_string = "IMMEDIATE";          break;
        case TokenType::FUNC_DECLARATION:   type_string = "FUNC_DECLARATION";   break;
        case TokenType::VAR_DEClARATION:    type_string = "VAR_DECLARATION";    break;
        case TokenType::LEFT_PARENTHESIS:   type_string = "LEFT_PARENTHESIS";   break;
        case TokenType::RIGHT_PARENTHESIS:  type_string = "RIGHT_PARENTHESIS";  break;
        case TokenType::LEFT_SCOPE:         type_string = "LEFT_SCOPE";         break;
        case TokenType::RIGHT_SCOPE:        type_string = "RIGHT_SCOPE";        break;
        case TokenType::IF_STATEMENT:       type_string = "IF_STATEMENT";       break;
        case TokenType::WHILE_STATEMENT:    type_string = "WHILE_STATEMENT";    break;
        case TokenType::RETURN_STATEMENT:   type_string = "RETURN_STATEMENT";   break;
        case TokenType::OP_ADD:             type_string = "OP_ADD";             break;
        case TokenType::OP_SUB:             type_string = "OP_SUB";             break;
        case TokenType::OP_MUL:             type_string = "OP_MUL";             break;
        case TokenType::OP_DIV:             type_string = "OP_DIV";             break;
        case TokenType::ASSIGNMENT:         type_string = "ASSIGNMENT";         break;
        case TokenType::CMP_LESS:           type_string = "CMP_LESS";           break;
        case TokenType::CMP_BIGGER:         type_string = "CMP_BIGGER";         break;
        case TokenType::CMP_EQUAL:          type_string = "CMP_EQUAL";          break;
        case TokenType::STATEMENT_END:      type_string = "STATEMENT_END";      break;
        case TokenType::END_OF_PROGRAM:     type_string = "END_OF_PROGRAM";     break;
        case TokenType::COMMA:              type_string = "COMMA";              break;
    }

    return (os << type_string);
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
    void                 advance        ();
    void                 skip_spaces    ();
    char                 current_char   ( int offset = 0) const;
    Token                read_immediate ();
    Token                read_word      ();
    std::optional<Token> read_symbol    ();

};

} // ! namespace lexer
} // ! namespace dumb

#endif // ! DUMB_LEXER_HH__
