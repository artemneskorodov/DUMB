#include <fstream>

#include "ast.hh"
#include "ast_dump.hh"

#include "dot_graph/graph.h"

namespace dumb
{
namespace ast
{
namespace dump
{

namespace
{

static constexpr std::string_view kImmediateNodeColor       = "#c40f0f";
static constexpr std::string_view kIdentifierNodeColor      = "#d8e610";
static constexpr std::string_view kBinaryOpNodeColor        = "#31cc12";
static constexpr std::string_view kAssignmentNodeColor      = "#16da95";
static constexpr std::string_view kIfNodeColor              = "#0ab2c8";
static constexpr std::string_view kWhileNodeColor           = "#383cb1";
static constexpr std::string_view kFunctionNodeColor        = "#7937ae";
static constexpr std::string_view kFunctionCallNodeColor    = "#b036b9";
static constexpr std::string_view kReturnNodeColor          = "#c53986";
static constexpr std::string_view kProgramNodeColor         = "#727272";
static constexpr std::string_view kNewVariableNodeColor     = "#96eb0c";
static constexpr std::string_view kNametableNodeColor       = "#176383";

class ASTDumper final : public Visitor
{
public:
    ASTDumper( const std::string& filename)
     :  graph_{ filename}
    {
    }

    const dot_graph::Graph&
    GetGraph() const &
    {
        return graph_;
    }

    void
    Visit( Immediate& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Immediate | { <Value> Value | " + std::to_string( node.value) + " } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kImmediateNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");

        graph_.addEdge( parent_node_id_, this_node_id);
    }

    void
    Visit( Identifier& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Identifier | <Name> " + std::to_string( node.id) + " }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kIdentifierNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");

        graph_.addEdge( parent_node_id_, this_node_id);
    }

    void
    Visit( BinaryOp& node) override
    {
        std::string this_node_id = get_node_id();
        std::string op_string{};
        switch ( node.operation )
        {
            case BinaryOp::OP_ADD:        op_string = "ADD";  break;
            case BinaryOp::OP_SUB:        op_string = "SUB";  break;
            case BinaryOp::OP_MUL:        op_string = "MUL";  break;
            case BinaryOp::OP_DIV:        op_string = "DIV";  break;
        }
        std::string label = "{ <Type> BinaryOp | { <Operation> Operation | " + op_string +
                            " } | { <Left> Left | <Right> Right } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kBinaryOpNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id);

        set_parent_id( this_node_id + ":Left");
        node.left.get()->Accept( *this);

        set_parent_id( this_node_id + ":Right");
        node.right.get()->Accept( *this);
    }

    void
    Visit( Assignment& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Assignment | { <Prev> Prev | { <Left> Identifier | " + std::to_string( node.left) + " } | <Right> Right | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kAssignmentNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        set_parent_id( this_node_id + ":Right");
        node.right.get()->Accept( *this);
    }

    void
    Visit( If& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> If | { <Prev> Prev | <Condition> Condition | <Body> Body | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kIfNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        dot_graph::Subgraph *old_subgraph = current_subgraph_;

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_if_condition_" + this_node_id)
                                          .setColor( "#646464");

        set_parent_id( this_node_id + ":Condition");

        std::string op_node_id = get_node_id();
        std::string op_string{};
        switch ( node.condition.operation )
        {
            case CompareOp::OP_CMP_LESS:   op_string = "LESS";   break;
            case CompareOp::OP_CMP_EQUAL:  op_string = "EQUAL";  break;
            case CompareOp::OP_CMP_BIGGER: op_string = "BIGGER"; break;
        }
        std::string cmp_label = "{ <Type> CompareOp | { <Operation> Operation | " + op_string +
                                " } | { <Left> Left | <Right> Right } }";

        current_subgraph_->addNode( op_node_id)
                          .setFillColor( kBinaryOpNodeColor)
                          .setQuoted( "label", cmp_label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( this_node_id + ":Condition", op_node_id);

        set_parent_id( op_node_id + ":Left");
        node.condition.left->Accept( *this);

        set_parent_id( op_node_id + ":Right");
        node.condition.right->Accept( *this);

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_if_body_" + this_node_id);

        set_parent_id( this_node_id + ":Body");
        for ( auto& stmt : node.body )
        {
            std::string current_stmt_id = next_node_id();
            stmt.get()->Accept( *this);
            set_parent_id( current_stmt_id + ":Next");
        }

        current_subgraph_ = old_subgraph;
    }

    void Visit( While& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> If | { <Prev> Prev | <Condition> Condition | <Body> Body | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kWhileNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        dot_graph::Subgraph *old_subgraph = current_subgraph_;

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_while_condition_" + this_node_id)
                                          .setColor( "#646464");

        set_parent_id( this_node_id + ":Condition");

        std::string op_node_id = get_node_id();
        std::string op_string{};
        switch ( node.condition.operation )
        {
            case CompareOp::OP_CMP_LESS:   op_string = "LESS";   break;
            case CompareOp::OP_CMP_EQUAL:  op_string = "EQUAL";  break;
            case CompareOp::OP_CMP_BIGGER: op_string = "BIGGER"; break;
        }
        std::string cmp_label = "{ <Type> CompareOp | { <Operation> Operation | " + op_string +
                                " } | { <Left> Left | <Right> Right } }";

        current_subgraph_->addNode( op_node_id)
                          .setFillColor( kBinaryOpNodeColor)
                          .setQuoted( "label", cmp_label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( this_node_id + ":Condition", op_node_id);

        set_parent_id( op_node_id + ":Left");
        node.condition.left->Accept( *this);

        set_parent_id( op_node_id + ":Right");
        node.condition.right->Accept( *this);

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_while_body_" + this_node_id);

        set_parent_id( this_node_id + ":Body");
        for ( auto& stmt : node.body )
        {
            std::string current_stmt_id = next_node_id();
            stmt.get()->Accept( *this);
            set_parent_id( current_stmt_id + ":Next");
        }

        current_subgraph_ = old_subgraph;
    }

    void
    Visit( FunctionCall& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> FunctionCall | <Name> " +
                            std::to_string( node.id) +
                            " | <Parameters> Parameters }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kFunctionCallNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id);

        dot_graph::Subgraph *old_subgraph = current_subgraph_;
        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_call_parameters_of_" + this_node_id)
                                          .setColor( "#646464");

        set_parent_id( this_node_id + ":Parameters");
        for ( auto& param : node.parameters )
        {
            std::string current_param_id = next_node_id();
            param.get()->Accept( *this);
            set_parent_id( current_param_id);
        }

        current_subgraph_ = old_subgraph;
    }

    void
    Visit( Return& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Return | { <Prev> Prev | <Expression> Expression | <Next> Next } }";
        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kReturnNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        set_parent_id( this_node_id + ":Expression");
        node.expression.get()->Accept( *this);
    }

    void
    Visit( Input& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Input | { <Prev> Prev | { <Identifier> Identifier | " +
                            std::to_string( node.identifier) + "} | { <String> String | " +
                            node.string + "} | <Next> Next }}";
        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kReturnNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev");
    }

    void
    Visit( Output& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Input | { <Prev> Prev | <Expression> Expression | { <String> String | " +
                            node.string + "} | <Next> Next }}";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kReturnNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev");

        set_parent_id( this_node_id + ":Expression");
        node.expression->Accept( *this);
    }

    void
    Visit( NewVariable& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> NewVariable | { <Prev> Prev | { <Identifier> Identifier | " + std::to_string( node.identifier) + " }| <Initializer> Initializer | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kNewVariableNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        if ( node.initializer != nullptr )
        {
            set_parent_id( this_node_id + ":Initializer");
            node.initializer.get()->Accept( *this);
        }
    }

    void
    DumpFunction( ast::Function *function)
    {
        // Function header node
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Function | <Name> " + std::to_string( function->id) +
                            " | { <Prev> Prev | <Parameters> Parameters | <Body> Body | <Next> Next } }";

        graph_.addNode( this_node_id)
              .setFillColor( kFunctionNodeColor)
              .setQuoted( "label", label)
              .setShape( "Mrecord")
              .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id);

        // Parameters
        std::string parameters_node_id = get_node_id();

        label = "{ ";
        for ( ir::VarID& param : function->parameters )
        {
            label += "{ Identifier | " + std::to_string( param) + " }";
            if ( &param != &function->parameters.back() )
            {
                label += " | ";
            }
        }
        label += "}";
        graph_.addNode( parameters_node_id)
              .setFillColor( kFunctionNodeColor)
              .setQuoted( "label", label)
              .setShape( "Mrecord")
              .setStyle( "filled");
        graph_.addEdge( this_node_id + ":Parameters", parameters_node_id);

        // Function body
        current_subgraph_ = &graph_.addSubgraph( "cluster_body_of_" + this_node_id)
                                   .setColor( "#646464");

        set_parent_id( this_node_id + ":Body");
        for ( auto& stmt : function->body )
        {
            std::string current_stmt_id = next_node_id();
            stmt.get()->Accept( *this);
            set_parent_id( current_stmt_id + ":Next");
        }
    }

    void
    DumpProgram( ast::Program *program)
    {
        std::string nametable{ ""};
        for ( const auto& it : program->nametable.GetNametable() )
        {
            std::string type{ ""};
            switch ( it.GetType() )
            {
                case nametable::Symbol::Type::FUNCTION:        type = "function";        break;
                case nametable::Symbol::Type::GLOBAL_VARIABLE: type = "global variable"; break;
                case nametable::Symbol::Type::LOCAL_VARIABLE:  type = "local variable";  break;
            }
            nametable += "{" + std::to_string( it.GetID()) + " | "+ type + " | " + it.GetName() + "}";
            if ( &it != &program->nametable.GetNametable().back() )
            {
                nametable += " | ";
            }
        }

        std::string nametable_node_id = get_node_id();
        graph_.addNode( nametable_node_id)
              .setFillColor( kNametableNodeColor)
              .setQuoted( "label", nametable)
              .setShape( "Mrecord")
              .setStyle( "filled");

        std::string this_node_id = get_node_id();

        std::string label = "{ <Type> Program }";
        graph_.addNode( this_node_id)
              .setFillColor( kProgramNodeColor)
              .setQuoted( "label", label)
              .setShape( "Mrecord")
              .setStyle( "filled");

        current_subgraph_ = &graph_.addSubgraph( "global_variables_" + this_node_id)
                                   .setColor( "#646464");

        for ( auto& var : program->global_variables )
        {
            set_parent_id( this_node_id);
            var->Accept( *this);
        }

        current_subgraph_ = nullptr;

        for ( auto& function : program->functions )
        {
            set_parent_id( this_node_id);
            DumpFunction( &function);
        }
    }

private:
    dot_graph::Graph     graph_             { "Program"};
    int                  nodes_counter_     { 0};
    dot_graph::Subgraph *current_subgraph_  { nullptr};
    std::string          parent_node_id_    {};

private:
    std::string get_node_id() { return "node_" + std::to_string( nodes_counter_++); }
    std::string next_node_id() const { return "node_" + std::to_string( nodes_counter_); }
    void set_parent_id( const std::string &id) { parent_node_id_ = id; }

};

} // ! anonymous namespace

void
DumpAST( ast::Program      *program,
         const std::string& output)
{
    ASTDumper dumper{ output};
    std::string nametable{ ""};
    dumper.DumpProgram( program);

    std::cout << dumper.GetGraph();

    dumper.GetGraph().translateWithDot( output, "svg");
}

} // ! namespace dump
} // ! namespace ast
} // ! namespace dumb
