#ifndef DUMB_AST_HH__
#define DUMB_AST_HH__

#include <memory>
#include <string>
#include <list>

#include "ir.hh"
#include "nametable.hh"

namespace dumb
{
namespace ast
{

class Visitor;

///
/// @brief
///
struct ASTNode
{
    virtual ~ASTNode() = default;
    virtual void Accept( Visitor& v) = 0;

};

using ASTNodePtr = std::unique_ptr<ASTNode>;

///
/// @brief
///
struct ExprNode : public ASTNode
{
    virtual ~ExprNode() = default;

};

using ExprNodePtr = std::unique_ptr<ExprNode>;

///
/// @brief
///
struct StmtNode : public ASTNode
{
    virtual ~StmtNode() = default;

};

using StmtNodePtr = std::unique_ptr<StmtNode>;

///
/// @brief
///
struct Immediate final : public ExprNode
{
public:
    explicit
    Immediate( hir::ImmType value)
     :  value{ value}
    {
    }

    void Accept( Visitor& v) override;

    hir::ImmType value;

};

///
/// @brief
///
struct Identifier final : public ExprNode
{
    explicit
    Identifier( std::size_t id)
     :  id{ id}
    {
    }

    void Accept( Visitor& v) override;

    hir::VarID id;

};

///
/// @brief
///
struct BinaryOp : public ExprNode
{
    enum Operation
    {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
    };

    explicit
    BinaryOp( Operation  op,
              ASTNodePtr left,
              ASTNodePtr right)
     :  operation { op},
        left      { std::move( left)},
        right     { std::move( right)}
    {
    }

    void Accept( Visitor& v) override;

    Operation  operation;
    ASTNodePtr left;
    ASTNodePtr right;

};

///
/// @brief
///
struct FunctionCall final : public ExprNode
{
    explicit
    FunctionCall( std::size_t            id,
                  std::list<ExprNodePtr> parameters)
     :  id         { id},
        parameters { std::move( parameters)}
    {
    }

    void Accept( Visitor& v) override;

    std::size_t            id;
    std::list<ExprNodePtr> parameters;

};


///
/// @brief
///
struct Assignment final : public StmtNode
{
    explicit
    Assignment( hir::VarID   left,
                ExprNodePtr right)
     :  left  { left},
        right { std::move( right)}
    {
    }

    void Accept( Visitor& v) override;

    hir::VarID   left;
    ExprNodePtr right;

};

///
/// @brief Compare operation used for while and if
///
struct CompareOp
{
    enum Operation
    {
        OP_CMP_LESS,
        OP_CMP_EQUAL,
        OP_CMP_BIGGER,
    };

    explicit
    CompareOp( Operation  op,
               ExprNodePtr left,
               ExprNodePtr right)
     :  operation { op},
        left      { std::move( left)},
        right     { std::move( right)}
    {
    }

    Operation   operation;
    ExprNodePtr left;
    ExprNodePtr right;

};

///
/// @brief
///
struct If final : public StmtNode
{
    explicit
    If( CompareOp              condition,
        std::list<StmtNodePtr> body)
     :  condition { std::move( condition)},
        body      { std::move( body)}
    {
    }

    void Accept( Visitor &v) override;

    CompareOp              condition;
    std::list<StmtNodePtr> body;

};

///
/// @brief
///
struct While final : public StmtNode
{
    explicit
    While( CompareOp              condition,
           std::list<StmtNodePtr> body)
     :  condition { std::move( condition)},
        body      { std::move( body)}
    {
    }

    void Accept( Visitor& v) override;

    CompareOp              condition;
    std::list<StmtNodePtr> body;

};

///
/// @brief
///
struct Return final : public StmtNode
{
    explicit
    Return( ExprNodePtr expression)
     :  expression( std::move( expression))
    {
    }

    void Accept( Visitor& v) override;

    ExprNodePtr expression;

};

///
/// @brief
///
struct NewVariable final : public StmtNode
{
    explicit
    NewVariable( hir::VarID   identifier,
                 ExprNodePtr initializer)
     :  identifier  { identifier},
        initializer { std::move( initializer)}
    {
    }

    void Accept( Visitor& v) override;

    hir::VarID   identifier;
    ExprNodePtr initializer;

};

///
/// @brief
///
struct Function final
{
    explicit
    Function( std::size_t            id,
              std::list<hir::VarID>   parameters,
              std::list<StmtNodePtr> body)
     :  id         { id},
        parameters { std::move( parameters)},
        body       { std::move( body)}
    {
    }

    hir::FuncID             id;
    std::list<hir::VarID>   parameters;
    std::list<StmtNodePtr> body;

};

///
/// @brief
///
struct Program final
{
    explicit
    Program( std::list<Function>    functions,
             std::list<StmtNodePtr> global_variables,
             nametable::NameTable   nametable)
     :  functions        { std::move( functions)},
        global_variables { std::move( global_variables)},
        nametable        { std::move( nametable)}
    {
    }

    std::list<Function>    functions;
    std::list<StmtNodePtr> global_variables;
    nametable::NameTable   nametable;

};

///
/// @brief
///
class Visitor
{
public:
    virtual void Visit( Immediate&    node) = 0;
    virtual void Visit( Identifier&   node) = 0;
    virtual void Visit( BinaryOp&     node) = 0;
    virtual void Visit( Assignment&   node) = 0;
    virtual void Visit( If&           node) = 0;
    virtual void Visit( While&        node) = 0;
    virtual void Visit( FunctionCall& node) = 0;
    virtual void Visit( Return&       node) = 0;
    virtual void Visit( NewVariable&  node) = 0;

};

///
/// @brief
///
inline void Immediate::Accept    ( Visitor& v) { v.Visit( *this); }
inline void Identifier::Accept   ( Visitor& v) { v.Visit( *this); }
inline void BinaryOp::Accept     ( Visitor& v) { v.Visit( *this); }
inline void Assignment::Accept   ( Visitor& v) { v.Visit( *this); }
inline void If::Accept           ( Visitor& v) { v.Visit( *this); }
inline void While::Accept        ( Visitor& v) { v.Visit( *this); }
inline void FunctionCall::Accept ( Visitor& v) { v.Visit( *this); }
inline void Return::Accept       ( Visitor& v) { v.Visit( *this); }
inline void NewVariable::Accept  ( Visitor& v) { v.Visit( *this); }

} // ! namespace ast
} // ! namespace dumb

#endif // ! DUMB_AST_HH__
