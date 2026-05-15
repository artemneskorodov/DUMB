#include <sstream>
#include <string>

#include "polish_notation.hh"
#include "ast.hh"

namespace dumb
{
namespace pn
{

namespace
{

class PolishNotationVisitor : public ast::ConstantVisitor
{
public:
    PolishNotationVisitor( const nt::NameTable& nametable)
     :  nametable_{ nametable}
    {
    }

public:
    void
    Visit( const ast::Immediate& node) override
    {
        ss_ << node.value;
    }

    void
    Visit( const ast::Identifier& node) override
    {
        const nt::Symbol* sym = nametable_.FindSymbol( node.id);
        ss_ << sym->GetName();
    }

    void
    Visit( const ast::BinaryOp& node) override
    {
        switch ( node.operation )
        {
            case ast::BinaryOp::Operation::OP_ADD: ss_ << "+ "; break;
            case ast::BinaryOp::Operation::OP_SUB: ss_ << "- "; break;
            case ast::BinaryOp::Operation::OP_MUL: ss_ << "* "; break;
            case ast::BinaryOp::Operation::OP_DIV: ss_ << "/ "; break;
        }

        ss_ << " ";
        node.left->Accept( *this);
        ss_ << " ";
        node.right->Accept( *this);
    }

    void
    Visit( const ast::Assignment& node) override
    {
        ss_ << ":= ";
        const nt::Symbol *left_sym = nametable_.FindSymbol( node.left);
        ss_ << left_sym->GetName();
        ss_ << " ";
        node.right->Accept( *this);
        ss_ << "\n";
    }

    void
    Visit( const ast::If& node) override
    {
        ss_ << "if ";
        visit_compare( node.condition);
        ss_ << " then\n";
        for ( const ast::StmtNodePtr& stmt : node.body )
        {
            stmt->Accept( *this);
        }
        ss_ << "endif\n";
    }

    void
    Visit( const ast::While& node) override
    {
        ss_ << "while ";
        visit_compare( node.condition);
        ss_ << " do\n";
        for ( const ast::StmtNodePtr& stmt : node.body )
        {
            stmt->Accept( *this);
        }
        ss_ << "endwhile\n";
    }

    void
    Visit( const ast::FunctionCall& node) override
    {
        const nt::Symbol *sym = nametable_.FindSymbol( node.id);

        ss_ << sym->GetName() << " ";

        for ( const ast::ExprNodePtr& param : node.parameters)
        {
            param->Accept( *this);
            if ( &param != &node.parameters.back() )
            {
                ss_ << " ";
            }
        }
    }

    void
    Visit( const ast::Return& node) override
    {
        ss_ << "ret";
        if ( node.expression != nullptr )
        {
            ss_ << " ";
            node.expression->Accept( *this);
        }
        ss_ << "\n";
    }

    void
    Visit( const ast::NewVariable& node) override
    {
        const nt::Symbol *sym = nametable_.FindSymbol( node.identifier);

        ss_ << "var " << sym->GetName();
        if ( node.initializer != nullptr )
        {
            ss_ << " ";
            node.initializer->Accept( *this);
        }
        ss_ << "\n";
    }

    void
    Visit( const ast::Input& node) override
    {
        const nt::Symbol *sym = nametable_.FindSymbol( node.identifier);
        ss_ << "input " << sym->GetName();
        ss_ << " \"" << node.string << "\"\n";
    }

    void
    Visit( const ast::Output& node) override
    {
        ss_ << "output ";
        node.expression->Accept( *this);
        ss_ << " \"" << node.string << "\"\n";
    }

public:
    template<typename T>
    PolishNotationVisitor&
    operator<<( const T& value)
    {
        ss_ << value;
        return *this;
    }

    std::string
    Str() const
    {
        return ss_.str();
    }

private:
    void
    visit_compare( const ast::CompareOp& cmp)
    {
        switch (cmp.operation)
        {
            case ast::CompareOp::OP_CMP_LESS:   ss_ << "<";  break;
            case ast::CompareOp::OP_CMP_EQUAL:  ss_ << "=="; break;
            case ast::CompareOp::OP_CMP_BIGGER: ss_ << ">";  break;
        }
        ss_ << " ";
        cmp.left->Accept(*this);
        ss_ << " ";
        cmp.right->Accept(*this);
    }

private:
    std::stringstream    ss_{};
    const nt::NameTable& nametable_;

};

} // ! anonymous namespace

std::string
GeneratePolishNotation( const ast::Program& ast)
{
    PolishNotationVisitor visitor{ ast.nametable};

    // Generating global variables
    for ( const ast::StmtNodePtr& global_variable : ast.global_variables )
    {
        global_variable->Accept( visitor);
    }

    for ( const ast::Function& function : ast.functions )
    {
        const nt::Symbol *func_sym = ast.nametable.FindSymbol( function.id);
        visitor << "func " << func_sym->GetName() << " ";
        for ( nt::SymbolID param_id : function.parameters )
        {
            const nt::Symbol *param_sym = ast.nametable.FindSymbol( param_id);
            visitor << param_sym->GetName() << " ";
        }
        visitor << "\n";

        for ( const ast::StmtNodePtr& stmt : function.body )
        {
            stmt->Accept( visitor);
        }

        visitor << "endfunc\n\n";
    }

    return visitor.Str();
}

} // ! namespace pn
} // ! namespace dumb
