#ifndef DUMB_OPTIONS_HH__
#define DUMB_OPTIONS_HH__

#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace dumb
{
namespace option
{

struct OptionBase
{
    OptionBase( std::string long_name,
                std::string short_name)
     :  long_name  { std::move( long_name)},
        short_name { std::move( short_name)}
    {
    }

    virtual void Read( const std::vector<std::string>& args, std::size_t& pos) = 0;
    virtual size_t Count() const = 0;

    virtual ~OptionBase() = default;

    std::string long_name;
    std::string short_name;

};

template<typename ValueT>
struct Option : OptionBase
{
    Option( std::string long_name,
            std::string short_name,
            ValueT default_value)
     :  OptionBase{ std::move( long_name), std::move( short_name)},
        value{ std::move( default_value)}
    {
    }

    void
    Read( const std::vector<std::string>& args,
          std::size_t& pos) override
    {
        std::stringstream value_stream( args[pos]);
        value_stream >> value;
        // Skipping value
        is_present = true;
    }

    size_t
    Count() const override
    {
        return 1;
    }

    ValueT value;
    bool is_present = false;

};

template<>
struct Option<bool> : OptionBase
{
    Option( std::string long_name,
            std::string short_name,
            bool default_value)
     :  OptionBase{ long_name, short_name},
        value{ default_value}
    {
    }

    void
    Read( const std::vector<std::string>& /*args*/,
          std::size_t& /*pos*/) override
    {
        is_present = true;
        value = true;
    }

    size_t
    Count() const override
    {
        return 0;
    }

    std::size_t count = 0;
    bool value;
    bool is_present = false;

};

template<typename Type>
using OptionPtr = std::unique_ptr<Option<Type>>;

class OptionsParser
{
public:
    template<typename ValueT>
    void
    AddOption( std::string long_name, std::string short_name, ValueT default_value)
    {
        options_.emplace_back( std::make_unique<Option<ValueT>>( long_name, short_name, default_value));
    }

    void
    ParseArgs( std::vector<std::string> args)
    {
        for ( std::size_t pos = 0; pos != args.size(); /* pos is moved inside the loop */ )
        {
            bool found_option = false;
            for ( std::unique_ptr<OptionBase>& opt : options_ )
            {
                if ( opt->long_name == args[pos] || opt->short_name == args[pos] )
                {
                    ++pos;
                    opt->Read( args, pos);
                    pos += opt->Count();
                    found_option = true;
                    break;
                }
            }
            if ( !found_option )
            {
                throw std::runtime_error{ "Unknown option: \"" + args[pos] + "\""};
            }
        }
    }

    template<typename ValueT>
    ValueT
    GetOption( const std::string& name)
    {
        for ( std::unique_ptr<OptionBase>& opt : options_ )
        {
            if ( opt->long_name == name || opt->short_name == name )
            {
                auto *opt_ptr = static_cast<Option<ValueT>*>( opt.get());
                return opt_ptr->value;
            }
        }
        throw std::runtime_error{ "No option names: \"" + name + "\""};
    }

private:
    std::vector<std::unique_ptr<OptionBase>> options_{};

};

} // ! namespace option
} // ! namespace dumb

#endif // ! DUMB_OPTIONS_HH__
