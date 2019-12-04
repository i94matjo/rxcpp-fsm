/*! \file  rx-fsm-state_machine.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_STATE_MACHINE_HPP)
#define RX_FSM_STATE_MACHINE_HPP

#include <unordered_map>

#include "rx-fsm-delegates.hpp"
#include "rx-fsm-pseudostate.hpp"
#include "rx-fsm-state.hpp"
#include "rx-fsm-transition.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

struct state_machine_delegate : public virtual_region_delegate
                              , public std::enable_shared_from_this<state_machine_delegate>
{

    typedef state_machine_delegate this_type;

    enum region_status_type
    {
        active,
        await_join,
        await_finalize
    };

    // "static" data
    typedef std::unordered_map<std::shared_ptr<virtual_vertex_delegate>, std::vector<std::shared_ptr<state_delegate>>> state_ancestor_map;
    state_ancestor_map state_ancestors;
    typedef std::unordered_map<std::shared_ptr<virtual_vertex_delegate>, observable<transition_delegate::transition_data>> state_observable_map;
    state_observable_map state_observable;
    typedef std::unordered_map<std::shared_ptr<pseudostate_delegate>, std::vector<std::shared_ptr<virtual_vertex_delegate>>> join_pseudostate_map;
    join_pseudostate_map join_pseudostates;
    std::unordered_set<std::shared_ptr<virtual_vertex_delegate>> target_states;
    std::atomic_bool assembled;

    // "dynamic" data
    struct current_state
    {
        std::weak_ptr<virtual_region_delegate> region;
        std::shared_ptr<virtual_vertex_delegate> state;
        region_status_type status;
        composite_subscription lifetime, state_lifetime;
        std::weak_ptr<current_state> parent;
        std::vector<std::shared_ptr<current_state>> children;
        bool entered;
    };
    typedef std::unordered_map<std::shared_ptr<pseudostate_delegate>, std::shared_ptr<virtual_vertex_delegate>> shallow_history_pseudostate_map;
    shallow_history_pseudostate_map shallow_history_pseudostates;
    typedef std::unordered_map<std::shared_ptr<pseudostate_delegate>, std::vector<std::shared_ptr<virtual_vertex_delegate>>> deep_history_pseudostate_map;
    deep_history_pseudostate_map deep_history_pseudostates;
    std::shared_ptr<current_state> current;
    composite_subscription subject_lifetime;
    subjects::subject<transition> subject;

    template<class Coordination>
    state_observable_map::mapped_type generate_observable_transitions(const Coordination& cn, const state_observable_map::key_type& state)
    {
       auto& transitions = state->transitions;
       if (transitions.empty()) {
           // no transitions
           return observable<>::never<transition_delegate::transition_data>();
       }
       std::vector<observable<transition_delegate::transition_data>> observables;
       for(const auto& t : transitions)
       {
           std::vector<std::shared_ptr<transition_delegate>> equally_triggered_transitions;
           equally_triggered_transitions.push_back(t);
           auto& ancestors = state_ancestors[state];
           for(auto it = ancestors.rbegin(); it != ancestors.rend(); ++it)
           {
               for(const auto& tt : (*it)->transitions)
               {
                   if(tt->equal_trigger(t))
                   {
                       equally_triggered_transitions.push_back(tt);
                   }
               }
           }
           observables.push_back(t->make_observable(equally_triggered_transitions));
       }
       return observable<>::iterate(observables, identity_immediate()).merge(identity_immediate()).observe_on(cn);
    }

    void get_join_pseudostates(const std::shared_ptr<virtual_vertex_delegate>& state);

    template<class Coordination>
    void generate_maps_recursively(const Coordination& cn, const state_ancestor_map::key_type& state, state_ancestor_map::mapped_type& ancestors)
    {
        state_ancestors[state] = ancestors;
        state_observable[state] = generate_observable_transitions(cn, state);
        for(const auto& t : state->transitions)
        {
            auto tgt = t->target();
            if (tgt) {
                target_states.insert(tgt);
            }
        }
        get_join_pseudostates(state);
        auto s = std::dynamic_pointer_cast<state_delegate>(state);
        if (s) {
            state_ancestor_map::mapped_type sub_state_ancestors = ancestors;
            sub_state_ancestors.push_back(s);
            for(const auto& region : s->regions)
            {
                for(const auto& sub_state : region->sub_states)
                {
                    generate_maps_recursively(cn, sub_state, sub_state_ancestors);
                }
            }
        }
    }

    template<class Coordination>
    void generate_maps(const Coordination& cn)
    {
        for(const auto& state : sub_states)
        {
            state_ancestor_map::mapped_type ancestors;
            generate_maps_recursively(cn, state, ancestors);
        }
    }

    std::shared_ptr<current_state> find_common_ancestor(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& target);

    void determine_exit_order_recursively(const std::shared_ptr<current_state>& current, std::multimap<int, const std::shared_ptr<current_state>>& map, int level);

    std::multimap<int, const std::shared_ptr<current_state>> determine_exit_order(const std::shared_ptr<current_state>& current);

    void get_deep_history_recursively(const std::shared_ptr<current_state>& current, std::vector<std::shared_ptr<virtual_vertex_delegate>>& history);

    std::vector<std::shared_ptr<virtual_vertex_delegate>> get_deep_history(const std::shared_ptr<current_state>& current);

    void exit_region(const std::shared_ptr<current_state>& current);

    void exit_state(const std::shared_ptr<current_state>& common, const std::shared_ptr<current_state>& current);

    void exit_states_recursively(const std::shared_ptr<current_state>& current);

    std::vector<std::shared_ptr<virtual_vertex_delegate>> determine_target_states(const std::shared_ptr<virtual_vertex_delegate>& target);

    void state_transition(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& target, const std::shared_ptr<transition_delegate::action>& action);

    std::shared_ptr<current_state> find_current_state(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& source_state) const;

    std::shared_ptr<current_state> find_current_region(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_region_delegate>& source_region) const;

    void enter_state(const std::shared_ptr<current_state>& current);

    void enter_states_recursively(const std::shared_ptr<current_state>& current, const std::vector<std::shared_ptr<virtual_vertex_delegate>>& target_states);

    void guard_executed(const std::shared_ptr<virtual_vertex_delegate>& state) const;

    void validate();

    template<class Coordination>
    observable<transition> assemble(Coordination cn)
    {
        auto self = this->shared_from_this();
        return observable<>::create<transition>([self, cn](subscriber<transition> subscr) {
            auto o = self->owner_.lock();
            if (o) {
                self->throw_exception<not_allowed>("cannot be assembled since it is a sub machine");
            }
            bool expected(false);
            if (!self->assembled.compare_exchange_strong(expected, true)) {
                // already assembled
                self->throw_exception<not_allowed>("already assembled");
            }
            if (self->sub_states.empty()) {
                self->throw_exception<not_allowed>("must have states");
            }
            self->generate_maps(cn);
            self->validate();
            auto initial = get_pseudostate(pseudostate_kind::initial, self->sub_states);
            if (!initial) {
                self->throw_exception<not_allowed>("has no initial state");
            }
            self->current = std::make_shared<current_state>();
            self->current->lifetime.add(self->subject_lifetime);
            self->current->region = self;
            self->current->status = active;
            std::vector<std::shared_ptr<virtual_vertex_delegate>> states(1, initial);
            auto cs = self->subject.get_observable().subscribe(subscr);
            self->current->lifetime.add(cs);
            self->current->entered = false;
            self->enter_states_recursively(self->current, states);
        }).subscribe_on(cn);
    }

    template<class Coordination>
    composite_subscription start(Coordination cn, const composite_subscription& cs)
    {
        return assemble(std::move(cn)).subscribe(cs, [](const fsm::transition&){}, [](std::exception_ptr e){ std::rethrow_exception(e); });
    }

    void find_reachable_states_recursively(std::unordered_map<std::shared_ptr<virtual_vertex_delegate>, bool>& states_reached, const std::shared_ptr<virtual_vertex_delegate>& state) const;

    std::vector<std::string> find_unreachable_states() const;

    virtual std::string type_name() const override;

    explicit state_machine_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit state_machine_delegate(std::string n);

    virtual ~state_machine_delegate() override;
};

}

/*!  \brief  A state machine comprises one or more regions, each region containing a graph (possibly hierarchical)
             comprising a set of vertices interconnected by arcs representing transitions.

     State machine execution is triggered by appropriate event occurrences. A particular execution of a state machine is
     represented by a set of valid path traversals through one or more region graphs, triggered by the dispatching of an
     event occurrence that match active triggers in these graphs. The rules for matching triggers are described below. In
     the course of such a traversal, a state machine instance may execute a potentially complex sequence of behaviors
     associated with the particular elements of the graphs that are being traversed (transition effects, state entry and
     state exit behaviors, etc.).

     In this implementation event occurences are represented by rx observable instances.

     \image html state_machine.png

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class state_machine final : public detail::virtual_region<detail::state_machine_delegate>
                          , public detail::state_machine_base
{
public:

    typedef state_machine this_type;

private:

    state_machine() = delete;

    explicit state_machine(std::shared_ptr<delegate_type> d);

    explicit state_machine(std::string n);

    friend state_machine make_state_machine(std::string);
    RX_FSM_FRIEND_OPERATORS(state_machine)

public:

    /*!  \return  True if the state machine is assembled.
     */
    bool is_assembled() const;

    /*!  \return  True if the state machine is terminated.
     */
    bool is_terminated() const;

    /*!  \brief  Terminates the state machine.

         After terminate is called the state machine will no longer repond to event occurrances.
     */
    void terminate();

    /*!  \brief  Finds unreachable states in an assembled state machine.

         \note State machine must be assembled.

         \return names of the states that are unreachable.
     */
    std::vector<std::string> find_unreachable_states() const;

    /*!  \brief  Assembles the state machine, using a specified coordination as event receiver.

         After the state machine has been defined (i.e. states and transitions are added), it must be assembled.
         The assemblation process includes a validation of the state machine, i.e. it checks that there are no
         invalid transition paths. If so exception will be raised. An observable of \a transition is returned from
         this function. When this observable is subscribed the state machine is started.
         All actions (including start up) will be executed on the supplied coordination \a cn. To avoid thread safety
         issues, you may very well use a thread safe single worker coordination, e.g. serialize_same_worker.

         \tparam Coordination  The coordination type of actions.

         \param cn  The coordination of actions.

         \return  An observable of transitions. Each transition taken will result in an on_next call.
     */
    template<class Coordination>
    auto assemble(Coordination cn)
        -> typename std::enable_if<is_coordination<Coordination>::value, observable<transition>>::type
    {
        return delegate->assemble<Coordination>(std::move(cn));
    }

    /*!  \brief Starts the state machine, using a specified coordination as event receiver.

         Essentially the same as \a assemble(Coordination), but instead of returning as observable,
         a composite_subscription is returned, subscribed to the observable of transitions. You can
         supply your own composite_subscription or one will be created for you.

         \tparam Coordination  The coordination type of the event receiver.

         \param cn  The coordination event receiver.
         \param cs  Optionally, use this composite_subscription.

         \return  A composite_subscription, subscribed to the observable of transitions, using the
                  supplied coordination.
     */
    template<class Coordination>
    auto start(Coordination cn, const composite_subscription& cs = composite_subscription())
        -> typename std::enable_if<is_coordination<Coordination>::value, composite_subscription>::type
    {
        return delegate->start<Coordination>(std::move(cn), cs);
    }

    /*!  \brief  Adds states to the state machine.

         \tparam State0  Type of the first state to add. Must be a vertex type (derived from \a state or \a pseudostate).
         \tparam StateN  Types of subsequent states to add, if any.

         \param state0  The first state to add.
         \param stateN  Subsequent states to add, if any.

         \return  A reference to self
     */
    template<class State0, class... StateN>
    auto with_state(State0 state0, StateN... stateN)
        -> typename std::enable_if<is_vertices<State0, StateN...>::value, this_type&>::type
    {
        return static_cast<this_type&>(detail::virtual_region<delegate_type>::with_sub_state(state0, stateN...));
    }

};

/*!  \brief Creates a state machine.

     \param name  The name of the state machine.

     \return  A \a state_machine instance.
 */
state_machine make_state_machine(std::string name);

RX_FSM_OPERATORS(state_machine)

template<class StateMachine>
auto state::with_state_machine(StateMachine state_machine)
    -> typename std::enable_if<is_state_machine<StateMachine>::value, this_type&>::type
{
    if (delegate->is_assembled()) {
        delegate->throw_exception<not_allowed>("state machine already assembled");
    }
    if (delegate->type == detail::state_delegate::simple) {
        delegate->type = detail::state_delegate::sub_machine;
    }
    if (delegate->type != detail::state_delegate::sub_machine) {
        delegate->throw_exception<not_allowed>("is not a sub machine state and is not allowed to have state machine");
    }
    auto sm = state_machine();
    if (sm->assembled.load()) {
        std::ostringstream msg;
        msg << "cannot add an aleady assembled state machine, '" << sm->name << "'";
        delegate->throw_exception<not_allowed>(msg.str());
    }
    delegate->regions.push_back(sm);
    sm->owner_ = delegate;
    return *this;
}

}
}

RX_FSM_HASH(rxcpp::fsm::state_machine)


#endif
