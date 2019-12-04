/*! \file  rx-fsm-pseudostate.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#pragma once

#if !defined(RX_FSM_PSEUDOSTATE_HPP)
#define RX_FSM_PSEUDOSTATE_HPP

#include "rx-fsm-delegates.hpp"
#include "rx-fsm-vertex.hpp"

namespace rxcpp {

namespace fsm {

/*!  \brief Pseudostate kind

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>
*/
enum class pseudostate_kind
{
    /*!  An initial pseudostate represents a starting point for a region; that is, it is the point from which
         execution of its contained behavior commences when the region is entered via default activation. It is the
         source for at most one transition, which may have an associated effect behavior, but not an associated trigger or
         guard. There can be at most one initial vertex in a region.

         \image html initial.png
     */
    initial,

    /*!  Entering a terminate pseudostate implies that the execution of the state machine is terminated
         immediately. The state machine does not exit any states nor does it perform any exit behaviors. Any executing
         activity behaviors are automatically aborted.

         \image html terminate.png
     */
    terminate,

    /*!  An entry point pseudostate represents an entry point for a state machine or a composite \a state that
         provides encapsulation of the insides of the state or state machine. In each region of the state machine or
         composite state owning the entry point, there is at most a single transition from the entry point to a vertex
         within that region.

         \note If the owning state has an associated entry behavior, this behavior is executed before any behavior
               associated with the outgoing transition. If multiple region are involved, the entry point acts as a fork
               pseudostate.

         \image html entry_point.png
     */
    entry_point,

    /*!  An exit point pseudostate is an exit point of a state machine or composite state that provides
         encapsulation of the insides of the state or state machine. Transitions terminating on an exit point within any
         region of the composite state or a state machine referenced by a submachine state implies exiting of this
         composite state or submachine state (with execution of its associated exit behavior). If multiple transitions
         from orthogonal regions within the state terminate on this pseudostate, then it acts like a join pseudostate.

         \image html exit_point.png
    */
    exit_point,

    /*!  This type of pseudostate is similar to a junction pseudostate and serves similar purposes,
         with the difference that the guard constraints on all outgoing transitions are evaluated dynamically, when the
         compound transition traversal reaches this pseudostate. Consequently, choice is used to realize a dynamic
         conditional branch. It allows splitting of compound transitions into multiple alternative paths such that the
         decision on which path to take may depend on the results of behavior executions performed in the same
         compound transition prior to reaching the choice point. If more than one guard evaluates to true, one of the
         corresponding transitions is selected. The algorithm for making this selection is not defined. If none of the
         guards evaluates to true, then the model is considered ill formed. To avoid this, it is recommended to define one
         outgoing transition with the predefined “else” guard for every choice pseudostate.

         \image html choice.png
     */
    choice,

    /*!  This type of pseudostate serves as a common target vertex for two or more transitions originating from
         vertices in different orthogonal regions. Transitions terminating on a join pseudostate cannot have a guard or a
         trigger. Similar to junction points in Petri nets, join pseudostates perform a synchronization function, whereby
         all incoming Transitions have to complete before execution can continue through an outgoing transition.

         \image html join.png
     */
    join,

    /*!  Fork pseudostates serve to split an incoming transition into two or more transitions terminating on
         vertices in orthogonal regions of a composite state. The transitions outgoing from a fork pseudostate cannot
         have a guard or a trigger.

         \image html fork.png
     */
    fork,

    /*!  This type of pseudostate is used to connect multiple transitions into compound paths between
         states. For example, a junction pseudostate can be used to merge multiple incoming transitions into a single
         outgoing transition representing a shared continuation path. Or, it can be used to split an incoming transition
         into multiple outgoing transition segments with different guard constraints.

         \note Such guard constraints are evaluated before any compound transition containing this pseudostate is
               executed, which is why this is referred to as a static conditional branch.

         It may happen that, for a particular compound transition, the configuration of transition paths and guard values
         is such that the compound transition is prevented from reaching a valid state configuration. In those cases, the
         entire compound transition is disabled even though its triggers are enabled. (As a way of avoiding this
         situation in some cases, it is possible to associate a predefined guard denoted as “else” with at most one
         outgoing transition. This transition is enabled if all the guards attached to the other transitions evaluate to
         false). If more than one guard evaluates to true, one of these is chosen. The algorithm for making this selection
         is not defined.

         \image html junction.png
     */
    junction,

    /*!  This type of pseudostate is a kind of variable that represents the most
         recent active substate of its containing region, but not the substates of that substate. A transition terminating
         on this pseudostate implies restoring the region to that substate with all the semantics of entering a state. A
         single outgoing transition from this pseudostate may be defined terminating on a substate of the composite
         state. This substate is the default shallow history state of the composite state. A shallow history pseudostate
         can only be defined for composite states and, at most one such pseudostate can be included in a region of a
         composite state.

         \image html shallow_history.png
     */
    shallow_history,

    /*!  This type of pseudostate is a kind of variable that represents the most recent active state
         configuration of its owning region. As explained above, a transition terminating on this pseudostate implies
         restoring the region to that same state configuration, but with all the semantics of entering a state.
         The entry behaviors of all states in the restored state configuration are
         performed in the appropriate order starting with the outermost state. A deep history pseudostate can only be
         defined for composite states and, at most one such pseudostate can be contained in a region of a composite
         state.

         \image html deep_history.png
     */
    deep_history
};

namespace detail {

struct state_delegate;
struct transition_delegate;

struct pseudostate_delegate : public virtual_vertex_delegate
{
    typedef pseudostate_delegate this_type;

    pseudostate_kind type;

    std::weak_ptr<state_delegate> history;

    virtual void check_transition_target(const std::shared_ptr<transition_delegate>& t) const override;

    virtual void check_transition_source(const std::shared_ptr<transition_delegate>& t) const override;

    static bool is_orthogonal_state(const std::shared_ptr<virtual_region_delegate>& region);

    static std::vector<std::shared_ptr<virtual_region_delegate>> orthogonal_regions(const std::shared_ptr<virtual_vertex_delegate>& orthogonal_state);

    virtual void validate() const override;

    virtual std::string type_name() const override;

    explicit pseudostate_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit pseudostate_delegate(std::string n);
};

std::shared_ptr<pseudostate_delegate> get_pseudostate(pseudostate_kind type, const std::vector<std::shared_ptr<virtual_vertex_delegate>>& states);

}

/*!  \brief  A pseudostate is an abstraction that encompasses different types of transient vertices in the state machine graph.

     Pseudostates are generally used to chain multiple transitions into more complex compound transitions. For
     example, by combining a transition entering a fork pseudostate with a set of transitions exiting that pseudostate, we get
     a compound transition that can enter a set of orthogonal regions.
     The specific semantics of a pseudostate depend on the kind of pseudostate, which is defined by its kind attribute of type
     \a pseudostate_kind.

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class pseudostate final : public detail::virtual_vertex<detail::pseudostate_delegate>
                        , public detail::pseudostate_base
{
public:

    typedef pseudostate this_type;

private:

    pseudostate() = delete;

    explicit pseudostate(std::shared_ptr<delegate_type> d);

    explicit pseudostate(pseudostate_kind t, std::string n);

    friend class state_machine;
    friend class transition;
    friend pseudostate make_pseudostate(pseudostate_kind, std::string);
    RX_FSM_FRIEND_OPERATORS(pseudostate)

public:

    /*!  \return  The pseudostate type according to \a pseudostate_kind
     */
    pseudostate_kind type() const;

    /*!  \brief  Adds a completion transition to another vertex.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).

         \param name    The name of the transition.
         \param target  The target vertex.

         \return  A reference to self.
     */
    template<class TargetState>
    auto with_transition(std::string name, TargetState target)
        -> typename std::enable_if<is_vertex<TargetState>::value, this_type&>::type
    {
        return static_cast<this_type&>(detail::virtual_vertex<delegate_type>::with_transition(name, target));
    }

    /*!  \brief  Adds a completion transition to another vertex with a specified action.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Action       The action callable type with signature void().

         \param name    The name of the transition.
         \param target  The target vertex.
         \param action  The action callable.

         \return  A reference to self.
     */
    template<class TargetState, class Action>
    auto with_transition(std::string name, TargetState target, Action action)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value>::value, this_type&>::type
    {
        return static_cast<this_type&>(detail::virtual_vertex<delegate_type>::with_transition(name, target, action));
    }

    /*!  \brief  Adds a completion transition to another vertex with a specified guard.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Guard        The guard callable type with signature bool().

         \param name    The name of the transition.
         \param target  The target vertex.
         \param guard   The guard callable.

         \return  A reference to self
     */
    template<class TargetState, class Guard>
    auto with_transition(std::string name, TargetState target, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        return static_cast<this_type&>(detail::virtual_vertex<delegate_type>::with_transition(name, target, guard));
    }

    /*!  \brief  Adds a completion transition to another vertex with a specified action and guard.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Action       The action callable type with signature void().
         \tparam Guard        The guard callable type with signature bool().

         \param name    The name of the transition.
         \param target  The target vertex.
         \param action  The action callable.
         \param guard   The guard callable.

         \return  A reference to self
     */
    template<class TargetState, class Action, class Guard>
    auto with_transition(std::string name, TargetState target, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        return static_cast<this_type&>(detail::virtual_vertex<delegate_type>::with_transition(name, target, action, guard));
    }

};

/*!  \brief Creates a pseudostate of type \a t.

     \param t     The pseudo state type.
     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_pseudostate(pseudostate_kind t, std::string name);

/*!  \brief Creates an initial pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_initial_pseudostate(std::string name);

/*!  \brief Creates a terminate pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_terminate_pseudostate(std::string name);

/*!  \brief Creates an entry point pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_entry_point_pseudostate(std::string name);


/*!  \brief Creates an exit point pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_exit_point_pseudostate(std::string name);

/*!  \brief Creates a choice pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_choice_pseudostate(std::string name);

/*!  \brief Creates a join pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_join_pseudostate(std::string name);

/*!  \brief Creates a fork pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_fork_pseudostate(std::string name);

/*!  \brief Creates a junction pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_junction_pseudostate(std::string name);

/*!  \brief Creates a shallow history pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_shallow_history_pseudostate(std::string name);

/*!  \brief Creates a deep history pseudostate.

     \param name  The name of the pseudostate.

     \return  A \a pseudostate instance.
 */
pseudostate make_deep_history_pseudostate(std::string name);

RX_FSM_OPERATORS(pseudostate)

}
}

RX_FSM_HASH(rxcpp::fsm::pseudostate)

#endif
