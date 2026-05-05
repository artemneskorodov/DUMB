#ifndef DUMB_LOGGER_HH__
#define DUMB_LOGGER_HH__

#include <string>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <array>
#include <string_view>

namespace dumb
{
namespace logger
{

namespace detail
{

enum class LogCategory : std::size_t
{
    LEXER          = 0,
    PARSER         = 1,
    IR_EMITTER     = 2,
    IR_INTERPRETER = 3,

    LOG_CATEGORY_MAX
};

constexpr std::size_t kLogCatMax = static_cast<std::size_t>( LogCategory::LOG_CATEGORY_MAX);

constexpr std::array<std::string_view, kLogCatMax> kLogCategoriesStrings{{
    "LEXER",
    "PARSER",
    "IR_EMITTER",
    "IR_INTERPRETER"
}};

inline std::string
LogCategoryToStr( LogCategory category)
{
    std::size_t category_int = static_cast<std::size_t>( category);
    if ( category_int >= kLogCatMax )
    {
        throw std::runtime_error{ "Unexpected LogCategory value = " +
                                  std::to_string( category_int)};
    }
    return std::string{ kLogCategoriesStrings[category_int]};
}

inline LogCategory
LogCategoryFromStr( const std::string value)
{
    for ( std::size_t i = 0; i != kLogCategoriesStrings.size(); ++i )
    {
        if ( kLogCategoriesStrings[i] == value )
        {
            return static_cast<LogCategory>( i);
        }
    }
    throw std::runtime_error{ "Unexpected log category: " + value};
}

inline std::array<bool, kLogCatMax> gLogCategories = {};

inline std::string
ShortPath( std::string path)
{
    std::size_t pos = path.size() - 1;
    std::size_t slash = path.size();
    while ( path.substr( pos, slash - pos) != "sources" )
    {
        --pos;
        if ( path[pos] == '/' || path[pos] == '\\' )
        {
            slash = pos;
            --pos;
        }
    }

    return path.substr( slash + 1, path.size() - slash - 1); // +- 1 for /
}

class Logger {
public:
    Logger( LogCategory category,
            std::string file,
            int         line,
            std::string func)
     :  category_ { category},
        file_     { std::move( file)},
        line_     { line},
        func_     { std::move( func)}
    {
    }

    ~Logger()
    {
        std::size_t category_int = static_cast<std::size_t>( category_);
        if ( category_int >= kLogCatMax )
        {
            std::cerr << "Unexpected category value = " << category_int;
            // Aborting as we are in the destructor
            abort();
        }

        if ( !gLogCategories[category_int] )
        {
            return ;
        }

        std::string category_str = LogCategoryToStr( category_);
        category_str.resize( 10, ' ');

        std::string path_str = ShortPath( file_) + ":" + std::to_string( line_) + ":" + func_;
        path_str.resize( 45, ' ');

        std::cout << "[" << category_str << "] ( " << path_str << " ): " << stream_.str() << std::endl;
    }

    template<typename ValueT> Logger&
    operator<<( const ValueT& value)
    {
        stream_ << value;
        return *this;
    }

private:
    LogCategory        category_;
    std::string        file_;
    int                line_;
    std::string        func_;
    std::ostringstream stream_;

};

} // ! namespace detail

inline void
ConfigureLogger( const std::string& categories)
{
    std::size_t pos = 0;
    while ( categories[pos] != '\0' )
    {
        if ( categories[pos] == ',' )
        {
            ++pos;
        }

        std::size_t end = pos;
        while ( categories[end] != '\0' && categories[end] != ',' )
        {
            ++end;
        }

        std::string category_str = categories.substr( pos, end - pos);
        detail::LogCategory category = detail::LogCategoryFromStr( category_str);
        detail::gLogCategories[static_cast<std::size_t>( category)] = true;

        pos = end;
    }
}

#define LOGGER( log_category_)                                                                     \
    ::dumb::logger::detail::Logger( ::dumb::logger::detail::LogCategory::log_category_,            \
                                    __FILE__, __LINE__, __FUNCTION__)                              \

} // ! namespace logger
} // ! namespace dumb

#endif // ! DUMB_LOGGER_HH__
