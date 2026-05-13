#ifndef DUMB_INDUCTIVE_VARIABLES_HH__
#define DUMB_INDUCTIVE_VARIABLES_HH__

#include <vector>

#include "ir.hh"

namespace dumb
{
namespace iv
{

///
/// @brief Structure to store information about basic induction.
///
/// @note Basic induction is variable initialized before loop and incremented (or decremented)
///       by same value each iteration.
///
struct BasicInductionVar
{
    ir::PhiNode *phi_node;
    ir::Loop *loop;
    ir::Operand init;
    ir::Operand step;
};

std::vector<BasicInductionVar> GetInductiveVariables( ir::Program& program);

} // ! namespace iv
} // ! namespace dumb

#endif // ! DUMB_INDUCTIVE_VARIABLES_HH__
