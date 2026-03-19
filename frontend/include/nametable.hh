#ifndef DUMB_NAMETABLE_HH__
#define DUMB_NAMETABLE_HH__

#include <string>
#include <vector>
#include <optional>

#include "ir.hh"

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
            hir::VarID          id)
     :  name_ { name},
        type_ { type},
        id_   { id}
    {
    }

    std::string GetName () const { return name_; }
    Type        GetType () const { return type_; }
    hir::VarID   GetID   () const { return id_;   }

private:
    std::string name_;
    Type        type_;
    hir::VarID   id_;

};

class NameTable final
{
public:
    hir::VarID
    AddSymbol( const std::string& name,
               Symbol::Type       type)
    {
        hir::VarID id = symbols_counter_;
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

    std::optional<Symbol>
    GetSymbol( const std::string& name) const &
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

    const Symbol *
    GetSymbol( hir::VarID id) const &
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

    hir::VarID
    GetMaxSymbolIndex() const
    {
        return symbols_counter_;
    }

    const std::vector<Symbol> &
    GetNametable() const &
    {
        return nametable_;
    }

private:
    std::vector<Symbol>        nametable_       {};
    std::vector<hir::VarID>     visible_names_   {};
    std::vector<std::size_t>   scope_symbols_   {};
    hir::VarID                  symbols_counter_ { 0};

};

} // ! namespace nametable
} // ! namespace dumb

#endif // ! DUMB_NAMETABLE_HH__
