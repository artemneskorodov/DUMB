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

inline std::string
basic_block_id( int id,
                int func_id)
{
    return "__BASIC_BLOCK_" + std::to_string( id) + "_IN_FUNC_" + std::to_string( func_id);
}

#if 0

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
        result_ = "FunctionCall: " + dest + " = call " + node.name + " (";
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

    std::string
    GetStr( const ir::BasicBlockTerminator& terminator)
    {
        if ( terminator.type == ir::CmpType::INVALID )
        {
            return "Invalid";
        }
        if ( terminator.type == ir::CmpType::ALWAYS_TRUE )
        {
            return "True";
        }
        std::string left_str  = op_dumper_.GetStr( *terminator.left);
        std::string right_str = op_dumper_.GetStr( *terminator.right);
        std::string cmp_str;
        switch ( terminator.type )
        {
            case ir::CmpType::LESS:   cmp_str = " &lt; ";  break;
            case ir::CmpType::EQUAL:  cmp_str =  " == "; break;
            case ir::CmpType::BIGGER: cmp_str = " &gt; ";  break;
            default: throw std::runtime_error{ "Unexpected compare type"};
        }

        return left_str + cmp_str + right_str;
    }

private:
    std::string   result_;
    OperandDumper op_dumper_;

};

#endif

class InstructionDumper
{
public:
    InstructionDumper( const ir::Program& program,
                       const ir::Function& function)
     :  program_{ program},
        function_{ function}
    {
    }

    std::pair<std::string, std::string>
    GetStr( const ir::Instruction& instr)
    {
        std::string opcode{};
        switch ( instr.opcode )
        {
            case ir::Opcode::ADD:    opcode = "add";    break;
            case ir::Opcode::SUB:    opcode = "sub";    break;
            case ir::Opcode::MUL:    opcode = "mul";    break;
            case ir::Opcode::DIV:    opcode = "div";    break;
            case ir::Opcode::MOV:    opcode = "mov";    break;
            case ir::Opcode::RET:    opcode = "ret";    break;
            case ir::Opcode::CALL:   opcode = "call";   break;
            case ir::Opcode::INPUT:  opcode = "input";  break;
            case ir::Opcode::OUTPUT: opcode = "output"; break;
            default: throw std::runtime_error{ "Unexpected instruction"};
        }
        std::string result = opcode + " { ";
        for ( std::size_t i = 0; i != instr.operands.size(); ++i )
        {
            result += get_operand( instr.operands[i]);
            if ( i + 1 != instr.operands.size() )
            {
                result += ", ";
            }
        }
        result += "}";
        return {get_operand( instr.defines), result};
    }

    std::string
    GetTerminatorStr( const ir::BasicBlockTerminator& terminator)
    {
        if ( terminator.type == ir::CmpType::INVALID )
        {
            return "Invalid";
        }
        if ( terminator.type == ir::CmpType::ALWAYS_TRUE )
        {
            return "True";
        }
        std::string left_str  = get_operand( terminator.left);
        std::string right_str = get_operand( terminator.right);
        std::string cmp_str;
        switch ( terminator.type )
        {
            case ir::CmpType::LESS:   cmp_str = " &lt; ";  break;
            case ir::CmpType::EQUAL:  cmp_str =  " == "; break;
            case ir::CmpType::BIGGER: cmp_str = " &gt; ";  break;
            default: throw std::runtime_error{ "Unexpected compare type"};
        }

        return left_str + cmp_str + right_str;
    }

private:
    std::string
    get_operand( const ir::Operand& operand)
    {
        switch ( operand.type )
        {
            case ir::Operand::EMPTY:
            {
                return "none()";
            }
            case ir::Operand::VARIABLE:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.value);
                return "var(" + sym->GetName() + ")";
            }
            case ir::Operand::GLOBAL:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.value);
                return "glob(" + sym->GetName() + ")";
            }
            case ir::Operand::IMMEDIATE:
            {
                return "imm(" + std::to_string( operand.value) + ")";
            }
            case ir::Operand::LABEL:
            {
                return "label(" + basic_block_id( operand.value, function_.Id()) + ")";
                return "label(" + std::to_string( operand.value) + ")";
            }
            case ir::Operand::FUNC_LABEL:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.value);
                return "func(" + sym->GetName() + ")";
            }
            case ir::Operand::STRING_LABEL:
            {
                return "str(" + program_.Strings()[operand.value] + ")";
            }
        }
    }

private:
    const ir::Program& program_;
    const ir::Function& function_;

};

std::string
dump_basic_block( const ir::Program& program,
                  const ir::Function& function,
                  const ir::BasicBlock& basic_block,
                  dot_graph::Graph& graph)
{
    InstructionDumper dumper{ program, function};

    html::HTMLTable result{};
    result.addRow().addCell( "BasicBlock_" + std::to_string( basic_block.id))
                   .setColSpan( 4)
                   .setPort( "Prev");

    result.addRow().addCell( "Predecessors").setColSpan( 4);

    for ( int pred : basic_block.predecessors )
    {
        result.addRow().addCell( "BasicBlock_" + std::to_string( pred)).setColSpan( 4);
    }

    result.addRow().addCell( "Instructions").setColSpan( 4);

    for ( size_t i = 0; i != basic_block.instructions.size(); ++i )
    {
        std::pair<std::string, std::string> instr_str = dumper.GetStr( basic_block.instructions[i]);
        html::HTMLRow& row = result.addRow();
        row.addCell( std::to_string( i));
        row.addCell( instr_str.first).setColSpan( 1);
        row.addCell( instr_str.second).setColSpan( 2);
    }

    html::HTMLRow& row = result.addRow();
    row.addCell( "Terminator")
       .setPort( "Next");
    row.addCell( dumper.GetTerminatorStr( basic_block.terminator));
    row.addCell( "True").setPort( "True");
    row.addCell( "False").setPort( "False");

    if ( basic_block.terminator.type != CmpType::INVALID )
    {
        graph.addEdge( basic_block_id( basic_block.id, function.Id()) + ":True",
                       basic_block_id( basic_block.terminator.true_dest, function.Id()) + ":Prev");

        if ( basic_block.terminator.type != CmpType::ALWAYS_TRUE )
        {
            graph.addEdge( basic_block_id( basic_block.id, function.Id()) + ":False",
                           basic_block_id( basic_block.terminator.false_dest, function.Id()) + ":Prev");
        }
    }

    return static_cast<std::string>( result);
}

void
dump_function( const ir::Program&   program,
               const ir::Function&  function,
               dot_graph::Graph&    graph,
               dot_graph::Subgraph& subgraph)
{
    html::HTMLTable func_start{};

    const nt::Symbol *sym = program.Nametable().FindSymbol( function.Id());

    html::HTMLRow& func_row = func_start.addRow();
    func_row.addCell( "Function " + sym->GetName())
            .setPort( "Prev");
    for ( int param_id : function.Params() )
    {
        const nt::Symbol *param_sym = program.Nametable().FindSymbol( param_id);
        html::HTMLRow& row = func_start.addRow();
        row.addCell( param_sym->GetName());
    }
    html::HTMLRow& start_row = func_start.addRow();
    start_row.addCell( "Start")
             .setPort( "Next");

    std::string start_id = "__START_OF_" + sym->GetName();

    subgraph.addNode( start_id)
            .setHtmlLabel( static_cast<std::string>( func_start))
            .setShape( "box");

    graph.addEdge( "__PROGRAM__:Functions", start_id + ":Prev");

    if ( !function.BasicBlocks().empty() )
    {
        std::string first_bb_id = basic_block_id( function.BasicBlocks()[0].id, function.Id());
        graph.addEdge( start_id + ":Next", first_bb_id + ":Prev");
    }

    for ( const BasicBlock& basic_block : function.BasicBlocks() )
    {
        subgraph.addNode( basic_block_id( basic_block.id, function.Id()))
                .setHtmlLabel( dump_basic_block( program, function, basic_block, graph))
                .setShape( "box");
    }
}

} // ! anonymous namespace

void
DumpIR( const ir::Program& program,
        const std::string& output)
{
    dot_graph::Graph graph{ "Program"};

    html::HTMLTable header{};
    html::HTMLRow& row = header.addRow();
    row.addCell( "Program header")
       .setColSpan( 2);

    html::HTMLRow& globals_header = header.addRow();
    globals_header.addCell( "Globals");
    globals_header.addCell( "Functions")
                  .setRowSpan( program.Globals().size() + 1)
                  .setPort( "Functions");

    for ( int global_id : program.Globals() )
    {
        const nt::Symbol *sym = program.Nametable().FindSymbol( global_id);
        html::HTMLRow& var_row = header.addRow();
        var_row.addCell( sym->GetName() + "(id = " + std::to_string( global_id) + ")");
    }

    graph.addNode( "__PROGRAM__")
         .setHtmlLabel( static_cast<std::string>( header))
         .setShape( "box");

    dot_graph::Subgraph& preamble = graph.addSubgraph( "cluster__PREAMBLE__")
                                         .setColor( kFunctionClusterColor);

    dump_function( program, program.Preamble(), graph, preamble);

    for ( const Function& func : program.Functions() )
    {
        dot_graph::Subgraph& func_graph = graph.addSubgraph( "cluster_function_" + std::to_string( func.Id()))
                                               .setColor( kFunctionClusterColor);
        dump_function( program, func, graph, func_graph);
    }

    graph.translateWithDot( output, "svg");
}

} // ! namespace dump
} // ! namespace ir
} // ! namespace dumb
