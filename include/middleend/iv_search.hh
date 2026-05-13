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

    std::string
    ToStr() const
    {
        return "init=" + init.ToStr() + ", step=" + step.ToStr() + ", loop=(header=" +
               std::to_string( loop->header) + ", latch=" + std::to_string( loop->latch) + ")";
    }
};

///
/// @brief Structure to store information about derived induction.
///
/// @note derived induction is something which changes only one time per loop in form:
///       x = CONST * i + CONST ( CONST can be later replaced with LOOP_INVARIANT,
///                               i is basic induction )
///
struct DerivedInductionVar
{
    BasicInductionVar *base;
    ir::Instruction   *add;
    ir::Instruction   *mul;
    ir::ImmType        multiplier;
    ir::ImmType        offset;

    std::string
    ToStr() const
    {
        return "base=[" + base->ToStr() + "], mul=" + std::to_string( multiplier) +
               ", add=" + std::to_string( offset);
    }
};

///
/// @brief Alias for vector of BasicInductionVar
///
using BasicIndVarList   = std::vector<BasicInductionVar>;

///
/// @brief Alias for vector of DerivedInductionVar
///
/// @warning Stores pointers to elements of BasicInductionVar
///
using DerivedIndVarList = std::vector<DerivedInductionVar>;

///
/// @brief       Create list of basic induction variables.
/// @param func  Function to create list for its CFG.
/// @param loops Loops information. Used only to iterate through loops.
/// @return      Vector of found basic inductions variables.
///
BasicIndVarList GetInductiveVariables( ir::Function& func, const loops::LoopsInfo& loops);

///
/// @brief                  Create list of derived induction variables.
/// @param func             Function to create list for its CFG.
/// @param loops            Loops information. Used only to iterate through loops.
/// @param basic_inductions Vector of basic induction variables.
/// @return                 Vector of found derived induction variables.
///
/// @note Derived induction variable is something in form x = i * CONST + CONST, where i
///       is basic inductive variable.
///
/// @warning Derived induction stores pointer to basic induction variable, so it will lead to
///          bad behavior if BasicIndVarList is deleted when DerivedIndVarList is used.
///
DerivedIndVarList GetDerivedInductiveVariables( ir::Function&           func,
                                                const loops::LoopsInfo& loops,
                                                BasicIndVarList&        basic_inductions);

} // ! namespace iv
} // ! namespace dumb

#endif // ! DUMB_INDUCTIVE_VARIABLES_HH__
