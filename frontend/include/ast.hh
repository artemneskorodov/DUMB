#ifndef DUMB_AST_HH__
#define DUMB_AST_HH__

#include <memory>
#include <string>
#include <list>

#include "nametable.hh"

namespace dumb
{
namespace ast
{

struct ASTNode;
class Visitor;
using ASTNodePtr = std::unique_ptr<ASTNode>;

///
/// @brief
///
struct ASTNode
{
    virtual ~ASTNode() = default;
    virtual void Accept( Visitor& v) = 0;

};

///
/// @brief
///
struct Immediate final : public ASTNode
{
public:
    explicit
    Immediate( int value)
     :  value{ value}
    {
    }

    void Accept( Visitor& v) override;

    int value;

};

///
/// @brief
///
struct Identifier final : public ASTNode
{
    explicit
    Identifier( std::size_t id)
     :  id{ id}
    {
    }

    void Accept( Visitor& v) override;

    std::size_t id;

};

///
/// @brief
///
struct BinaryOp : public ASTNode
{
    enum Operation
    {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_CMP_LESS,
        OP_CMP_EQUAL,
        OP_CMP_BIGGER,
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
struct Assignment final : public ASTNode
{
    explicit
    Assignment( ASTNodePtr left,
                ASTNodePtr right)
     :  left  { std::move( left)},
        right { std::move( right)}
    {
    }

    void Accept( Visitor& v) override;

    ASTNodePtr left;
    ASTNodePtr right;

};

///
/// @brief
///
struct If final : public ASTNode
{
    explicit
    If( ASTNodePtr            condition,
        std::list<ASTNodePtr> body)
     :  condition { std::move( condition)},
        body      { std::move( body)}
    {
    }

    void Accept( Visitor &v) override;

    ASTNodePtr            condition;
    std::list<ASTNodePtr> body;

};

///
/// @brief
///
struct While final : public ASTNode
{
    explicit
    While( ASTNodePtr            condition,
           std::list<ASTNodePtr> body)
     :  condition { std::move( condition)},
        body      { std::move( body)}
    {
    }

    void Accept( Visitor& v) override;

    ASTNodePtr            condition;
    std::list<ASTNodePtr> body;

};

///
/// @brief
///
struct Return final : public ASTNode
{
    explicit
    Return( ASTNodePtr expression)
     :  expression( std::move( expression))
    {
    }

    void Accept( Visitor& v) override;

    ASTNodePtr expression;

};

///
/// @brief
///
struct FunctionCall final : public ASTNode
{
    explicit
    FunctionCall( std::size_t           id,
                  std::list<ASTNodePtr> parameters)
     :  id         { id},
        parameters { std::move( parameters)}
    {
    }

    void Accept( Visitor& v) override;

    std::size_t           id;
    std::list<ASTNodePtr> parameters;

};

///
/// @brief
///
struct Function final : public ASTNode
{
    explicit
    Function( std::size_t           id,
              std::list<ASTNodePtr> parameters,
              std::list<ASTNodePtr> body)
     :  id         { id},
        parameters { std::move( parameters)},
        body       { std::move( body)}
    {
    }

    void Accept( Visitor& v) override;

    std::size_t           id;
    std::list<ASTNodePtr> parameters;
    std::list<ASTNodePtr> body;

};

///
/// @brief
///
struct Program final : public ASTNode
{
    explicit
    Program( std::list<ASTNodePtr> functions,
             std::list<ASTNodePtr> global_variables,
             nametable::NameTable  nametable)
     :  functions        { std::move( functions)},
        global_variables { std::move( global_variables)},
        nametable        { std::move( nametable)}
    {
    }

    void Accept( Visitor& v) override;

    std::list<ASTNodePtr> functions;
    std::list<ASTNodePtr> global_variables;
    nametable::NameTable  nametable;

};

///
/// @brief
///
struct NewVariable final : public ASTNode
{
    explicit
    NewVariable( ASTNodePtr identifier,
                 ASTNodePtr initializer)
     :  identifier  { std::move( identifier)},
        initializer { std::move( initializer)}
    {
    }

    void Accept( Visitor& v) override;

    ASTNodePtr identifier;
    ASTNodePtr initializer;

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
    virtual void Visit( Function&     node) = 0;
    virtual void Visit( Program&      node) = 0;
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
inline void Function::Accept     ( Visitor& v) { v.Visit( *this); }
inline void Program::Accept      ( Visitor& v) { v.Visit( *this); }
inline void Return::Accept       ( Visitor& v) { v.Visit( *this); }
inline void NewVariable::Accept  ( Visitor& v) { v.Visit( *this); }

} // ! namespace ast
} // ! namespace dumb

#endif // ! DUMB_AST_HH__
