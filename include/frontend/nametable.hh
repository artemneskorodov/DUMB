#ifndef DUMB_NAMETABLE_HH__
#define DUMB_NAMETABLE_HH__

#include <string>
#include <vector>
#include <optional>

#include "ir.hh"

namespace dumb
{
namespace nt
{

using SymbolID = std::size_t;

enum class SymbolType
{
    FUNCTION,
    GLOBAL_VARIABLE,
    LOCAL_VARIABLE,
};

class Symbol final
{
public:
    Symbol( const std::string& name,
            SymbolType         type,
            SymbolID           id)
     :  name_ { name},
        type_ { type},
        id_   { id}
    {
    }

    std::string GetName     () const { return name_; }
    SymbolType  GetType     () const { return type_; }
    SymbolID    GetID       () const { return id_;   }
    std::string GetSafeName () const { return name_ + "__nt_id_" + std::to_string( id_); }
private:
    std::string name_;
    SymbolType  type_;
    SymbolID    id_;

};

class NameTable final
{
public:
    SymbolID
    AddSymbol( const std::string& name,
               SymbolType         type)
    {
        SymbolID id = symbols_counter_;
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
    HasScope() const
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

    ///
    /// @brief Find symbol in nametable by name in current scope.
    /// @param name Name of symbol
    /// @return Pointer to Symbol
    /// @warning This function can only be used before syntax analysis, as it is only looks for names in visible_names_
    /// @todo Fix this making different classes for syntax parser nametable and other nametables, or use std::string after syntax parsing
    ///
    const Symbol *
    GetSymbol( const std::string& name) const &
    {
        for ( auto it = visible_names_.rbegin(); it != visible_names_.rend(); ++it )
        {
            if ( nametable_[*it].GetName() == name )
            {
                return &nametable_[*it];
            }
        }
        return nullptr;
    }

    const Symbol *
    GetSymbol( SymbolID id) const &
    {
        for ( auto& sym : nametable_ )
        {
            if ( sym.GetID() == id )
            {
                return &sym;
            }
        }
        return nullptr;
    }

    const std::vector<Symbol> &
    GetNametable() const &
    {
        return nametable_;
    }

private:
    std::vector<Symbol>        nametable_       {};
    std::vector<SymbolID>      visible_names_   {};
    std::vector<std::size_t>   scope_symbols_   {};
    SymbolID                   symbols_counter_ { 0};

};

} // ! namespace nametable
} // ! namespace dumb

#endif // ! DUMB_NAMETABLE_HH__
