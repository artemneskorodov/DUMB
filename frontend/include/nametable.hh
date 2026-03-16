#ifndef DUMB_NAMETABLE_HH__
#define DUMB_NAMETABLE_HH__

#include <string>
#include <vector>
#include <optional>

namespace dumb
{
namespace nametable
{

class Symbol final
{
public:
    enum class Type
    {
        FUNCTION,
        GLOBAL_VARIABLE,
        LOCAL_VARIABLE,
    };

    Symbol( const std::string& name,
            Type               type,
            std::size_t        id)
     :  name_ { name},
        type_ { type},
        id_   { id}
    {
    }

    std::string GetName() const { return name_; }
    Type GetType() const { return type_; }
    std::size_t GetID() const { return id_; }

private:
    std::string name_;
    Type        type_;
    std::size_t id_;

};

class NameTable final
{
public:
    std::size_t
    AddSymbol( const std::string& name,
               Symbol::Type       type)
    {
        std::size_t id = symbols_counter_;
        ++symbols_counter_;
        nametable_.emplace_back( Symbol{ name, type, id});

        visible_names_.emplace_back( id);
        if ( HasScope() )
        {
            scope_symbols_.back() += 1;
        }
        return id;
    }

    void
    EnterScope()
    {
        scope_symbols_.emplace_back( 0);
    }

    bool
    HasScope()
    {
        return (scope_symbols_.size() != 0);
    }

    void
    LeaveScope()
    {
        for ( size_t i = 0; i != scope_symbols_.back(); ++i )
        {
            visible_names_.pop_back();
        }
        scope_symbols_.pop_back();
    }

    std::optional<Symbol>
    GetSymbol( const std::string& name) &
    {
        for ( auto it = visible_names_.rbegin(); it != visible_names_.rend(); ++it )
        {
            if ( nametable_[*it].GetName() == name )
            {
                return nametable_[*it];
            }
        }
        return std::nullopt;
    }

    const std::vector<Symbol> &
    GetNametable() const &
    {
        return nametable_;
    }

private:
    std::vector<Symbol>        nametable_       {};
    std::vector<std::size_t>   visible_names_   {};
    std::vector<std::size_t>   scope_symbols_   {};
    std::size_t                symbols_counter_ { 0};

};

} // ! namespace nametable
} // ! namespace dumb

#endif // ! DUMB_NAMETABLE_HH__
