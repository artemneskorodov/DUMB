#ifndef DUMB_EMIT_IR_HH__
#define DUMB_EMIT_IR_HH__

#include "ir.hh"
#include "ast.hh"

namespace dumb
{
namespace emit_ir
{

ir::Program EmitIR( ast::ASTNodePtr program);

};
};

#endif // ! DUMB_EMIT_IR_HH__
