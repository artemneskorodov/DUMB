#ifndef DUMB_EMIT_IR_HH__
#define DUMB_EMIT_IR_HH__

#include "ir.hh"
#include "ast.hh"

namespace dumb
{
namespace emit_ir
{

hir::Program EmitIR( ast::Program *program);

};
};

#endif // ! DUMB_EMIT_IR_HH__
