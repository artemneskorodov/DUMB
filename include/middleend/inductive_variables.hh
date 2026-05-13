#ifndef DUMB_INDUCTIVE_VARIABLES_HH__
#define DUMB_INDUCTIVE_VARIABLES_HH__

#include <vector>

#include "ir.hh"
#include "loops_search.hh"

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
    loops::Loop *loop;
    ir::Operand  init;
    ir::Operand  step;
};

std::vector<BasicInductionVar> GetInductiveVariables( ir::Function&           func,
                                                      const loops::LoopsInfo& loops);

} // ! namespace iv
} // ! namespace dumb

#endif // ! DUMB_INDUCTIVE_VARIABLES_HH__
