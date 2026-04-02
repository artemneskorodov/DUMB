#ifndef DUMB_OPTIONS_HH__
#define DUMB_OPTIONS_HH__

#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <memory>

namespace dumb
{
namespace option
{

struct OptionBase
{
    OptionBase( std::string long_name,
                std::string short_name,
                bool        required)
     :  long_name  { std::move( long_name)},
        short_name { std::move( short_name)},
        required   { required}
    {
    }

    virtual void Read( const std::vector<std::string>& args, std::size_t& pos) = 0;
    virtual size_t Count() const = 0;

    virtual ~OptionBase() = default;

    std::string long_name;
    std::string short_name;
    bool        required;
    bool        is_present = false;

};

template<typename ValueT>
struct Option : OptionBase
{
    Option( std::string long_name,
            std::string short_name,
            ValueT      default_value,
            bool        required)
     :  OptionBase{ std::move( long_name), std::move( short_name), required},
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

};

template<>
struct Option<bool> : OptionBase
{
    Option( std::string long_name,
            std::string short_name,
            bool        default_value,
            bool        required)
     :  OptionBase{ std::move( long_name), std::move( short_name), required},
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

};

class OptionsParser
{
public:
    template<typename ValueT>
    void
    AddOption( std::string long_name, std::string short_name, ValueT default_value, bool required)
    {
        options_.emplace_back( std::make_unique<Option<ValueT>>( long_name,
                                                                 short_name,
                                                                 default_value,
                                                                 required));
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
        for ( std::unique_ptr<OptionBase>& opt : options_ )
        {
            if ( opt->required && !opt->is_present )
            {
                throw std::runtime_error{ "Option \"" + opt->long_name + "\" is required"};
            }
        }
    }

    template<typename ValueT>
    ValueT
    GetOption( const std::string& name) const
    {
        for ( const std::unique_ptr<OptionBase>& opt : options_ )
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
