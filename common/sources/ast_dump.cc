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
        std::string label = "{ <Type> Immediate | { <Value> Value | " + std::to_string(node.value) + " } }";

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
        std::string label = "{ <Type> Identifier | <Name> " + node.name + " }";

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
            case BinaryOp::OP_CMP_LESS:   op_string = "LESS";  break;
            case BinaryOp::OP_CMP_EQUAL:  op_string = "EQUAL"; break;
            case BinaryOp::OP_CMP_BIGGER: op_string = "BIGGER";  break;
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
        std::string label = "{ <Type> Assignment | { <Prev> Prev | <Left> Left | <Right> Right | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kAssignmentNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        set_parent_id( this_node_id + ":Left");
        node.left.get()->Accept( *this);

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
                                          .setColor( "#646464")
                                          .setRaw( "rank", "same");

        set_parent_id( this_node_id + ":Condition");
        node.condition.get()->Accept( *this);

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

    void Visit( While& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> While | { <Prev> Prev | <Condition> Condition | <Body> Body | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kWhileNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        dot_graph::Subgraph *old_subgraph = current_subgraph_;

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_while_condition_" + this_node_id)
                                          .setColor( "#646464")
                                          .setRaw( "rank", "same");

        set_parent_id( this_node_id + ":Condition");
        node.condition.get()->Accept( *this);

        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_while_body_" + this_node_id)
                                          .setColor( "#646464")
                                          .setRaw( "rank", "same");

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
        std::string label = "{ <Type> FunctionCall | <Name> " + node.name + " | <Parameters> Parameters }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kFunctionCallNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id);

        dot_graph::Subgraph *old_subgraph = current_subgraph_;
        current_subgraph_ = &old_subgraph->addSubgraph( "cluster_call_parameters_of_" + this_node_id)
                                          .setColor( "#646464")
                                          .setRaw( "rank", "same");

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
    Visit( Function& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Function | <Name> " + node.name + " | { <Prev> Prev | <Parameters> Parameters | <Body> Body | <Next> Next } }";

        graph_.addNode( this_node_id)
              .setFillColor( kFunctionNodeColor)
              .setQuoted( "label", label)
              .setShape( "Mrecord")
              .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id);

        current_subgraph_ = &graph_.addSubgraph( "cluster_parameters_of_" + this_node_id)
                                   .setColor( "#646464")
                                   .setRaw( "rank", "same");

        set_parent_id( this_node_id + ":Parameters");
        for ( auto& param : node.parameters )
        {
            std::string current_param_id = next_node_id();
            param.get()->Accept( *this);
            set_parent_id( current_param_id);
        }

        current_subgraph_ = &graph_.addSubgraph( "cluster_body_of_" + this_node_id)
                                   .setColor( "#646464")
                                   .setRaw( "rank", "same");

        set_parent_id( this_node_id + ":Body");
        for ( auto& stmt : node.body )
        {
            std::string current_stmt_id = next_node_id();
            stmt.get()->Accept( *this);
            set_parent_id( current_stmt_id + ":Next");
        }
    }

    void
    Visit( Program& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> Program }";
        graph_.addNode( this_node_id)
              .setFillColor( kProgramNodeColor)
              .setQuoted( "label", label)
              .setShape( "Mrecord")
              .setStyle( "filled");

        for ( auto& function : node.functions )
        {
            set_parent_id( this_node_id);
            function.get()->Accept( *this);
        }
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
    Visit( NewVariable& node) override
    {
        std::string this_node_id = get_node_id();
        std::string label = "{ <Type> NewVariable | { <Prev> Prev | <Identifier> Identifier | <Initializer> Initializer | <Next> Next } }";

        current_subgraph_->addNode( this_node_id)
                          .setFillColor( kNewVariableNodeColor)
                          .setQuoted( "label", label)
                          .setShape( "Mrecord")
                          .setStyle( "filled");
        graph_.addEdge( parent_node_id_, this_node_id + ":Prev")
              .setColor( "#e600ff");

        set_parent_id( this_node_id + ":Identifier");
        node.identifier.get()->Accept( *this);

        if ( node.initializer != nullptr )
        {
            set_parent_id( this_node_id + ":Initializer");
            node.initializer.get()->Accept( *this);
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
DumpAST( ast::ASTNodePtr&   root,
         const std::string& output)
{
    ASTDumper dumper{ output};
    root.get()->Accept( dumper);

    std::cout << dumper.GetGraph();

    dumper.GetGraph().translateWithDot( output, "svg");
}

} // ! namespace dump
} // ! namespace ast
} // ! namespace dumb
