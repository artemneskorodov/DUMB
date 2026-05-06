#include <iostream>

#include "ir_dump.hh"
#include "ir.hh"
#include "utils.hh"
#include "logger.hh"

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

static std::string gDumpDir = "./";

constexpr std::string_view kFunctionClusterColor = "#75b560";

inline std::string
basic_block_id( int id,
                int func_id)
{
    return "__BASIC_BLOCK_" + std::to_string( id) + "_IN_FUNC_" + std::to_string( func_id);
}

class InstructionDumper
{
public:
    InstructionDumper( const ir::Program& program)
     :  program_{ program}
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
            result += GetOperandStr( instr.operands[i]);
            if ( i + 1 != instr.operands.size() )
            {
                result += ", ";
            }
        }
        result += "}";
        return { GetOperandStr( instr.defines), result};
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
        std::string left_str  = GetOperandStr( terminator.left);
        std::string right_str = GetOperandStr( terminator.right);
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

    std::string
    GetOperandStr( const ir::Operand& operand)
    {
        switch ( operand.type )
        {
            case ir::Operand::EMPTY:
            {
                return "none()";
            }
            case ir::Operand::VARIABLE:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.id);
                return "var(" + sym->GetName() + "." + std::to_string( operand.value) + ", id = " + std::to_string( operand.id) + ")";
            }
            case ir::Operand::GLOBAL:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.id);
                return "glob(" + sym->GetName() + ")";
            }
            case ir::Operand::IMMEDIATE:
            {
                return "imm(" + std::to_string( operand.value) + ")";
            }
            case ir::Operand::FUNC_LABEL:
            {
                const nt::Symbol *sym = program_.Nametable().FindSymbol( operand.id);
                return "func(" + sym->GetName() + ", id = " + std::to_string( operand.id) + ")";
            }
            case ir::Operand::STRING_LABEL:
            {
                return "str(" + program_.Strings()[operand.value] + ")";
            }
            default:
            {
                throw std::runtime_error{ "Unexpected operand type"};
            }
        }
    }

private:
    const ir::Program& program_;

};

std::string
dump_basic_block( const ir::Program& program,
                  const ir::Function& function,
                  const ir::BasicBlock& basic_block,
                  dot_graph::Graph& graph)
{
    InstructionDumper dumper{ program};

    html::HTMLTable result{};
    result.addRow().addCell( "BasicBlock_" + std::to_string( basic_block.id))
                   .setColSpan( 4)
                   .setPort( "Prev");

    result.addRow().addCell( "PHI acceptors").setColSpan( 4);
    for ( int succ : basic_block.phi_acceptors )
    {
        result.addRow().addCell( "BasicBlock_" + std::to_string( succ)).setColSpan( 4);
    }

    result.addRow().addCell( "Predecessors").setColSpan( 4);
    for ( BasicBlockID id : basic_block.predecessors )
    {
        result.addRow().addCell( "BasicBlock_" + std::to_string( id)).setColSpan( 4);
    }

    result.addRow().addCell( "Phi nodes").setColSpan( 4);

    for ( const ir::PhiNode& phi : basic_block.phi_nodes )
    {
        std::string name = dumper.GetOperandStr( ir::Operand{ ir::Operand::VARIABLE, phi.version, phi.var_id});
        std::string placeholder = "PHI{ ";
        for ( const auto& variants : phi.mapping )
        {
            placeholder += dumper.GetOperandStr( variants.second) + ":BB_" + std::to_string( variants.first) + " ";
        }
        placeholder += "}";
        html::HTMLRow& row = result.addRow();
        row.addCell( name);
        row.addCell( placeholder).setColSpan( 3);
    }

    result.addRow().addCell( "Instructions").setColSpan( 4);

    std::size_t counter = 0;
    for ( const ir::Instruction& instr : basic_block.instructions )
    {
        std::pair<std::string, std::string> instr_str = dumper.GetStr( instr);
        html::HTMLRow& row = result.addRow();
        row.addCell( std::to_string( counter++));
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
    func_row.addCell( "Function " + sym->GetName() +
                      "(id=" + std::to_string( function.Id()) + ")")
            .setPort( "Prev");
    for ( const SSAKey& param_id : function.Params() )
    {
        const nt::Symbol *param_sym = program.Nametable().FindSymbol( param_id.id);
        html::HTMLRow& row = func_start.addRow();
        row.addCell( param_sym->GetName() + "." + std::to_string( param_id.version));
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
        std::string first_bb_id = basic_block_id( function.GetBasicBlock( 0).id, function.Id());
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
ConfigureDump( const std::string& dir)
{
    gDumpDir = dir;
}

void
DumpIR( const ir::Program& program,
        const std::string& output)
{
    if ( !logger::CategoryEnabled( logger::LogCategory::DUMP_IR) )
    {
        return ;
    }

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

    for ( const Function& func : program.Functions() )
    {
        dot_graph::Subgraph& func_graph = graph.addSubgraph( "cluster_function_" + std::to_string( func.Id()))
                                               .setColor( kFunctionClusterColor);
        dump_function( program, func, graph, func_graph);
    }

    std::string filename = gDumpDir + "/" + utils::GetSafeTimeFilename() + "_" + output + ".svg";
    graph.translateWithDot( filename, "svg");

    LOGGER(DUMP_IR) << "Dump of IR with name \"" << output << "\" is saved to file \""
                    << filename << "\"";
}

} // ! namespace dump
} // ! namespace ir
} // ! namespace dumb
