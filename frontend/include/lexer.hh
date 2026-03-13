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
};

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
