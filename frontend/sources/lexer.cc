#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

#include "lexer.hh"

namespace dumb
{
namespace lexer
{

namespace
{

struct KeyWordInfo
{
    constexpr
    KeyWordInfo( const std::string_view &string,
                 TokenType        type)
     :  string { string},
        type   { type}
    {
    }

    std::string_view string;
    TokenType        type;

};

constexpr std::array<KeyWordInfo, 5> kKeyWordsAlpha
{{
    KeyWordInfo{ "function", TokenType::FUNC_DECLARATION },
    KeyWordInfo{ "variable", TokenType::VAR_DEClARATION  },
    KeyWordInfo{       "if", TokenType::IF_STATEMENT     },
    KeyWordInfo{    "while", TokenType::WHILE_STATEMENT  },
    KeyWordInfo{   "return", TokenType::RETURN_STATEMENT },
}};

constexpr std::array<KeyWordInfo, 11> kKeyWordsNormalSymbols
{{
    KeyWordInfo{ "+", TokenType::OP_ADD            },
    KeyWordInfo{ "-", TokenType::OP_SUB            },
    KeyWordInfo{ "*", TokenType::OP_MUL            },
    KeyWordInfo{ "/", TokenType::OP_DIV            },
    KeyWordInfo{ ">", TokenType::CMP_BIGGER        },
    KeyWordInfo{ "<", TokenType::CMP_LESS          },
    KeyWordInfo{ ";", TokenType::STATEMENT_END     },
    KeyWordInfo{ "(", TokenType::LEFT_PARENTHESIS  },
    KeyWordInfo{ ")", TokenType::RIGHT_PARENTHESIS },
    KeyWordInfo{ "{", TokenType::LEFT_SCOPE        },
    KeyWordInfo{ "}", TokenType::RIGHT_SCOPE       },
}};

} // ! anonymous namespace

Lexer::Lexer( const std::string &source)
 :  source_{ source}
{
    while ( pos_ != source_.size() )
    {
        skip_spaces();

        if ( std::isdigit( current_char()) )
        {
            tokens_.emplace_back( read_immediate());
        } else if ( std::isalpha( current_char()) ||
                    ( current_char() == '_') )
        {
            tokens_.emplace_back( read_word());
        } else
        {
            auto token = read_symbol();
            if ( !token.has_value() )
            {
                if ( current_char() == '\0' )
                {
                    break;
                } else
                {
                    std::string error = "Unexpected token: (line = " + std::to_string( line_) +
                                        ", column = " + std::to_string( column_) + ", symbol = " +
                                        std::string( {current_char()}) + " (" +
                                        std::to_string( current_char()) + ")\n";
                    throw std::runtime_error{ error};
                }
            }
            tokens_.emplace_back( token.value());
        }
    }

    tokens_.emplace_back( Token{ TokenType::END_OF_PROGRAM, "EOF", line_, column_});
}

std::vector<Token>
Lexer::Tokenize()
{
    return tokens_;
}

void
Lexer::advance()
{
    if ( current_char() == '\n' )
    {
        ++line_;
        column_ = 1;
    } else
    {
        ++column_;
    }
    ++pos_;
}

void
Lexer::skip_spaces()
{
    while ( std::isspace( current_char()) )
    {
        advance();
    }
}

char
Lexer::current_char( int offset) const
{
    if ( pos_ + offset >= source_.size() )
    {
        return '\0';
    }
    return source_[pos_ + offset];
}

Token
Lexer::read_immediate()
{
    assert( std::isdigit( current_char()));

    int line = line_;
    int column = column_;

    size_t start = pos_;
    advance();
    while ( std::isdigit( current_char()) )
    {
        advance();
    }
    std::string value = source_.substr( start, pos_ - start);
    return Token{ TokenType::IMMEDIATE, value, line, column};
}

Token
Lexer::read_word()
{
    assert( std::isalpha( current_char()) || (current_char() == '_'));

    int line = line_;
    int column = column_;

    size_t start = pos_;
    advance();
    while ( std::isalpha( current_char()) ||
            std::isdigit( current_char()) ||
            (current_char() == '_') )
    {
        advance();
    }
    std::string value = source_.substr( start, pos_ - start);
    TokenType type;
    bool found_key_word = false;
    for ( const auto &key_word_info : kKeyWordsAlpha )
    {
        if ( key_word_info.string == value )
        {
            type           = key_word_info.type;
            found_key_word = true;
            break;
        }
    }
    if ( !found_key_word )
    {
        type = TokenType::USER_STRING;
    }

    return Token{ type, value, line, column};
}

std::optional<Token>
Lexer::read_symbol()
{
    TokenType type;
    std::string value;

    int line = line_;
    int column = column_;

    if ( current_char() == '=' )
    {
        if ( current_char( 1) == '=' )
        {
            type  = TokenType::CMP_EQUAL;
            value = "==";
        } else
        {
            type  = TokenType::ASSIGNMENT;
            value = "=";
        }
    } else
    {
        bool found_symbol = false;
        for ( const auto& key_word : kKeyWordsNormalSymbols )
        {
            if ( current_char() == key_word.string[0] )
            {
                type         = key_word.type;
                value        = key_word.string;
                found_symbol = true;
                break;
            }
        }
        if ( !found_symbol )
        {
            return std::nullopt;
        }
    }

    pos_ += value.size();
    line_ += value.size();
    return Token{ type, value, line, column};
}

} // ! namespace lexer
} // ! namespace dumb
