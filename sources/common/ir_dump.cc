#include <iostream>

#include "ir_dump.hh"
#include "ir.hh"

#include "dot_graph/html.h"
#include "dot_graph/graph.h"

namespace dumb
{
namespace ir
{
namespace dump
{

namespace
{

constexpr std::string_view kFunctionClusterColor = "#75b560";

class OperandDumper : public ConstantOperandVisitor
{
public:
    void
    Visit( const VarOperand& node) override
    {
        result_ = node.name;
    }

    void
    Visit( const GVarOperand& node) override
    {
        result_ = node.name;
    }

    void
    Visit( const ImmOperand& node) override
    {
        result_ = "imm(" + std::to_string( node.value) + ")";
    }

    std::string
    GetStr( const ir::Operand& operand)
    {
        operand.Accept( *this);
        return result_;
    }

private:
    std::string result_;

};

class InstructionDumper : public ir::ConstantInstructionVisitor
{
public:
    void
    Visit( const ir::BinaryOpInstr& node) override
    {
        std::string left   { op_dumper_.GetStr( *node.first)};
        std::string right  { op_dumper_.GetStr( *node.second)};
        std::string dest   { op_dumper_.GetStr( *node.dest)};
        std::string op_str {};
        switch ( node.op )
        {
            case ir::BinaryOpType::ADD:         op_str = " + "; break;
            case ir::BinaryOpType::SUB:         op_str = " - "; break;
            case ir::BinaryOpType::MUL:         op_str = " * "; break;
            case ir::BinaryOpType::DIV:         op_str = " / "; break;
        }
        result_ = "BinaryOp: " + dest + " = " + left + op_str + right;
    }

    void
    Visit( const ir::UnaryOpInstr& node) override
    {
        if ( node.op == ir::UnaryOpType::MOV )
        {
            std::string operand = op_dumper_.GetStr( *node.operand);
            std::string dest    = op_dumper_.GetStr( *node.dest);
            result_ = "UnaryOp: " + dest + " = " + operand;
        } else if ( node.op == UnaryOpType::RET )
        {
            std::string operand = "";
            if ( node.operand != nullptr )
            {
                operand = op_dumper_.GetStr( *node.operand);
            }
            result_ = "UnaryOp: RET " + operand;
        } else
        {
            throw std::runtime_error{ "Unexpected unary operand"};
        }
    }

    void
    Visit( const ir::FunctionCallInstr& node) override
    {
        std::string dest = op_dumper_.GetStr( *node.dest);
        result_ = "FunctionCall: " + dest + " call " + node.name + " (";
        for ( auto& it : node.params )
        {
            std::string param = op_dumper_.GetStr( *it);
            result_ += param;
            if ( &it != &node.params.back() )
            {
                result_ += ", ";
            }
        }
        result_ += ")";
    }

    void
    Visit( const ir::CmpAndJmpInstr& node) override
    {
        if ( node.type == ir::CmpType::ALWAYS_TRUE )
        {
            result_ = "Jmp: goto BasicBlock_" + std::to_string( node.true_dest);
            return ;
        }

        std::string left_str = op_dumper_.GetStr( *node.left);
        std::string right_str = op_dumper_.GetStr( *node.right);
        std::string cmp_str;
        switch ( node.type )
        {
            case ir::CmpType::LESS:   cmp_str = " @lt ";  break;
            case ir::CmpType::EQUAL:  cmp_str = " == "; break;
            case ir::CmpType::BIGGER: cmp_str = " @gt ";  break;
            case ir::CmpType::ALWAYS_TRUE: break;
        }

        result_ = "CmpAndJmp: if ( " + left_str + cmp_str + right_str + " ) "
                  "goto BasicBlock_" + std::to_string( node.true_dest) +
                  " else goto BasicBlock_" + std::to_string( node.false_dest);
    }

    void
    Visit( const ir::InputInstr& node) override
    {
        result_ = "input (" + op_dumper_.GetStr( *node.dest) + ", \"" + node.string + "\")";
    }

    void
    Visit( const ir::OutputInstr& node) override
    {
        result_ = "output (" + op_dumper_.GetStr( *node.expression) + ", \"" + node.string + "\")";
    }

    std::string
    GetStr( const ir::Instruction& instr)
    {
        instr.Accept( *this);
        return result_;
    }

private:
    std::string   result_;
    OperandDumper op_dumper_;

};

std::string
dump_basic_block( const ir::BasicBlock& basic_block)
{
    InstructionDumper dumper{};

    html::HTMLTable result{};
    result.addRow()
          .addCell( "BasicBlock_" + std::to_string( basic_block.id))
          .setColSpan( 2)
          .setPort( "Prev");

    for ( size_t i = 0; i != basic_block.instructions.size(); ++i )
    {
        std::string instr_string = dumper.GetStr( *basic_block.instructions[i]);
        html::HTMLRow& row = result.addRow();
        row.addCell( std::to_string( i));
        row.addCell( instr_string);
    }

    result.addRow()
          .addCell( "Next")
          .setColSpan( 2)
          .setPort( "Next");

    return static_cast<std::string>( result);
}

void
dump_function( const ir::Function& function,
               dot_graph::Subgraph& subgraph,
               dot_graph::Graph& graph)
{
    html::HTMLTable func_start{};

    html::HTMLRow& func_row = func_start.addRow();
    func_row.addCell( "Function " + function.name)
            .setPort( "Prev");
    for ( const auto& param : function.params )
    {
        html::HTMLRow& row = func_start.addRow();
        row.addCell( param);
    }
    html::HTMLRow& start_row = func_start.addRow();
    start_row.addCell( "Start")
             .setPort( "Next");

    std::string prev_basic_block_id = "__START_OF_" + function.name;

    subgraph.addNode( prev_basic_block_id)
            .setHtmlLabel( static_cast<std::string>( func_start))
            .setShape( "box");

    graph.addEdge( "__PROGRAM__:Functions", prev_basic_block_id + ":Prev");

    for ( const auto& basic_block : function.basic_blocks )
    {
        std::string basic_block_id = "__BASIC_BLOCK_" + std::to_string( basic_block->id);
        subgraph.addNode( basic_block_id)
                .setHtmlLabel( dump_basic_block( *basic_block))
                .setShape( "box");
        graph.addEdge( prev_basic_block_id + ":Next", basic_block_id + ":Prev");
        prev_basic_block_id = std::move( basic_block_id);
    }
}

} // ! anonymous namespace

void
DumpIR( const ir::Program& program)
{
    dot_graph::Graph graph{ "Program"};

    html::HTMLTable header{};
    html::HTMLRow& row = header.addRow();
    row.addCell( "Program header")
       .setColSpan( 2);

    html::HTMLRow& globals_header = header.addRow();
    globals_header.addCell( "Globals");
    globals_header.addCell( "Functions")
                  .setRowSpan( program.globals.size() + 1)
                  .setPort( "Functions");

    for ( const std::string& var : program.globals )
    {
        html::HTMLRow& var_row = header.addRow();
        var_row.addCell( var);
    }

    graph.addNode( "__PROGRAM__")
         .setHtmlLabel( static_cast<std::string>( header))
         .setShape( "box");

    dot_graph::Subgraph& preamble = graph.addSubgraph( "cluster__PREAMBLE__")
                                         .setColor( kFunctionClusterColor);

    dump_function( *program.preamble, preamble, graph);

    for ( const auto& it : program.functions )
    {
        dot_graph::Subgraph& func = graph.addSubgraph( "cluster_function_" + it->name)
                                         .setColor( kFunctionClusterColor);
        dump_function( *it, func, graph);
    }

    graph.translateWithDot( "output1.svg", "svg");
}

} // ! namespace dump
} // ! namespace ir
} // ! namespace dumb
