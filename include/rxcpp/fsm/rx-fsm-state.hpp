/*! \file  rx-fsm-state.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_STATE_HPP)
#define RX_FSM_STATE_HPP

#include "rx-fsm-delegates.hpp"
#include "rx-fsm-pseudostate.hpp"
#include "rx-fsm-region.hpp"
#include "rx-fsm-vertex.hpp"
#include "rx-fsm-transition.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

struct state_delegate : public virtual_vertex_delegate
                      , public std::enable_shared_from_this<state_delegate>
{
    enum state_t
    {
        simple,
        composite,
        orthogonal,
        sub_machine
    };

    state_t type;

    typedef std::function<void()> on_entry_or_exit_t;

    on_entry_or_exit_t entry, exit;

    std::vector<std::shared_ptr<virtual_region_delegate>> regions;

    virtual void check_transition_target(const std::shared_ptr<transition_delegate>&) const override;

    virtual void check_transition_source(const std::shared_ptr<transition_delegate>&) const override;

    static inline void validate_sub_states(const std::shared_ptr<virtual_region_delegate>& region);

    virtual void validate() const override;

    void on_entry();

    void on_exit();

    bool contains(const std::shared_ptr<virtual_region_delegate>& region) const;

    virtual std::string type_name() const override;

    explicit state_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit state_delegate(std::string n);
};

struct final_state_delegate : public virtual_vertex_delegate
{
    virtual void check_transition_target(const std::shared_ptr<transition_delegate>&) const override;

    virtual void check_transition_source(const std::shared_ptr<transition_delegate>&) const override;

    virtual void validate() const override;

    virtual std::string type_name() const override;

    explicit final_state_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit final_state_delegate(std::string n);
};

}

class state_machine;

/*!  \brief  A state models a situation in the execution of a state machine behavior during which some invariant condition holds.

     The following kinds of States are distinguished:
       - simple state
       - composite state
       - submachine state

     A simple state has no internal vertices or transitions.

     \image html state.png

     A composite state contains at least one region.

     \image html composite_state.png

     A submachine state refers to an entire state machine, which is, conceptually, deemed to be “nested” within the state.

     \image html submachine_state.png

     A composite state can be either a simple composite state with exactly one region or an orthogonal state with multiple
     regions.

     \image html orthogonal_state.png

     Any state enclosed within a region of a composite state is called a substate of that composite state. It is called a direct
     substate when it is not contained in any other state; otherwise, it is referred to as an indirect substate.

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class state final : public detail::virtual_vertex<detail::state_delegate>
                  , public detail::state_base
{
public:

    typedef state this_type;

private:

    state() = delete;

    explicit state(std::shared_ptr<delegate_type> d);
    
    explicit state(std::string n);

    bool check_region(const std::shared_ptr<detail::virtual_region_delegate>& region);

    template<class... RegionN>
    struct WithRegion;

    template<class Region0>
    struct WithRegion<Region0>
    {
        this_type& operator()(this_type* self, Region0 region0)
        {
            auto r = region0();
            if (self->check_region(r)) {
                self->delegate->regions.push_back(r);
                r->owner_ = self->delegate;
            } else {
                std::ostringstream msg;
                msg << "has a region '" << r->name << "' that is equal to or has the same name as a sibling region";
                self->delegate->throw_exception<not_allowed>(msg.str());
            }
            return *self;
        }
    };

    template<class Region0, class... RegionN>
    struct WithRegion<Region0, RegionN...>
    {
        this_type& operator()(this_type* self, Region0 region0, RegionN... regionN)
        {
            WithRegion<Region0>()(self, std::move(region0));
            WithRegion<RegionN...>()(self, std::move(regionN)...);
            return *self;
        }
    };

    friend class state_machine;
    friend class transition;
    friend state make_state(std::string);
    RX_FSM_FRIEND_OPERATORS(state)

public:
    typedef detail::state_delegate::state_t state_t;

    /*!  \return  True if the state is simple, i.e. no sub state machine, no sub states, no orthogonal regions.
     */
    bool is_simple() const;

    /*!  \return  True if the state is composite, i.e. has sub states.
     */
    bool is_composite() const;

    /*!  \return  True if the state is orthogonal, i.e. has orthogonal regions.
     */
    bool is_orthogonal() const;

    /*!  \return  True if the state is a sub machine state, i.e. has a sub state machine.
     */
    bool is_sub_machine() const;

    /*!  \brief  Adds an on entry action to the state.

         \tparam OnEntry  Callable type.

         \param on_entry  The callable object

         \return  A reference to self.
     */
    template<class OnEntry>
    auto with_on_entry(OnEntry on_entry)
        -> typename std::enable_if<is_on_entry_or_exit<OnEntry>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        if (delegate->entry) {
            delegate->throw_exception<not_allowed>("has already specified an entry action");
        }
        delegate->entry = std::move(on_entry);
        return *this;
    }

    /*!  \brief  Adds an on exit action to the state.

         \tparam OnExit  Callable type.

         \param on_exit  The callable object

         \return  A reference to self.
     */
    template<class OnExit>
    auto with_on_exit(OnExit on_exit)
        -> typename std::enable_if<is_on_entry_or_exit<OnExit>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        if (delegate->exit) {
            delegate->throw_exception<not_allowed>("has already specified an exit action");
        }
        delegate->exit = std::move(on_exit);
        return *this;
    }

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

    /*!  \brief  Adds a triggered transition to another vertex.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Trigger      The trigger type, an observable.

         \param name     The name of the transition.
         \param target   The target vertex.
         \param trigger  The trigger observable.

         \return  A reference to self.
     */
    template<class TargetState, class Trigger>
    auto with_transition(std::string name, TargetState target, Trigger trigger)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_observable<Trigger>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = make_transition(false, std::move(name), delegate, target, std::move(trigger), detail::empty_action<Trigger>(), detail::empty_guard<Trigger>());
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a triggered transition to another vertex with a specified action.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Trigger      The trigger type, an observable.
         \tparam Action       The action callable type with signature void().

         \param name     The name of the transition.
         \param target   The target vertex.
         \param trigger  The trigger observable.
         \param action   The action callable.

         \return  A reference to self.
     */
    template<class TargetState, class Trigger, class Action>
    auto with_transition(std::string name, TargetState target, Trigger trigger, Action action)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_observable<Trigger>::value,
                                                 is_action_of<rxu::value_type_t<Trigger>, Action>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, target, std::move(trigger), std::move(action), detail::empty_guard<Trigger>());
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a triggered transition to another vertex with a specified guard.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Trigger      The trigger type, an observable.
         \tparam Guard        The guard callable type with signature bool().

         \param name     The name of the transition.
         \param target   The target vertex.
         \param trigger  The trigger observable.
         \param guard    The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Trigger, class Guard>
    auto with_transition(std::string name, TargetState target, Trigger trigger, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_observable<Trigger>::value,
                                                 is_guard_of<rxu::value_type_t<Trigger>, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, target, std::move(trigger), detail::empty_action<Trigger>(), std::move(guard));
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a triggered transition to another vertex with a specified action and guard.

         \tparam TargetState  The target vertex type (\a pseudostate or \a state).
         \tparam Trigger      The trigger type, an observable.
         \tparam Action       The action callable type with signature void().
         \tparam Guard        The guard callable type with signature bool().

         \param name     The name of the transition.
         \param target   The target vertex.
         \param trigger  The trigger observable.
         \param action   The action callable.
         \param guard    The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Trigger, class Action, class Guard>
    auto with_transition(std::string name, TargetState target, Trigger trigger, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_observable<Trigger>::value,
                                                 is_action_of<rxu::value_type_t<Trigger>, Action>::value,
                                                 is_guard_of<rxu::value_type_t<Trigger>, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, target, std::move(trigger), std::move(action), std::move(guard));
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a timeout transition to another vertex of a specified coordination.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Coordination  The type of coordination of which to perform the timeout.

         \param name      The name of the transition.
         \param target    The target vertex.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.

         \return  A reference to self.
     */
    template<class TargetState, class Coordination>
    auto with_transition(std::string name, TargetState target, Coordination cn, rxsc::scheduler::clock_type::duration duration)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_coordination<Coordination>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, target, std::move(cn), std::move(duration), detail::empty_action<void>(), detail::empty_guard<void>());
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a timeout transition to another vertex of a specified coordination with a specified action.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Action        The action callable type with signature void().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param action    The action callable.

         \return  A reference to self.
     */
    template<class TargetState, class Coordination, class Action>
    auto with_transition(std::string name, TargetState target, Coordination cn, rxsc::scheduler::clock_type::duration duration, Action action)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_coordination<Coordination>::value,
                                                 is_action_of<void, Action>::value>::value, this_type&>::type
    {
        auto t = detail::make_transition(false, std::move(name), delegate, target, std::move(cn), std::move(duration), std::move(action), detail::empty_guard<void>());
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a timeout transition to another vertex of a specified coordination with a specified guard.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Coordination, class Guard>
    auto with_transition(std::string name, TargetState target, Coordination cn, rxsc::scheduler::clock_type::duration duration, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_coordination<Coordination>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, target, std::move(cn), std::move(duration), detail::empty_action<void>(), std::move(guard));
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a timeout transition to another vertex of a specified coordination with a specified action and guard.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Action        The action callable type with signature void().
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param action    The action callable.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Coordination, class Action, class Guard>
    auto with_transition(std::string name, TargetState target, Coordination cn, rxsc::scheduler::clock_type::duration duration, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_coordination<Coordination>::value,
                                                 is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, target, std::move(cn), std::move(duration), std::move(action), std::move(guard));
        delegate->check_transition_source(t);
        target()->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds a timeout transition to another vertex using the identity_current_thread coordination.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).

         \param name      The name of the transition.
         \param target    The target vertex.
         \param duration  The timeout duration.

         \return  A reference to self.
     */
    template<class TargetState>
    auto with_transition(std::string name, TargetState target, rxsc::scheduler::clock_type::duration duration)
        -> typename std::enable_if<is_vertex<TargetState>::value, this_type&>::type
    {
        return with_transition(std::move(name), std::move(target), identity_current_thread(), std::move(duration));
    }

    /*!  \brief  Adds a timeout transition to another vertex using the identity_current_thread coordination with a specified action.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Action        The action callable type with signature void().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param duration  The timeout duration.
         \param action    The action callable.

         \return  A reference to self.
     */
    template<class TargetState, class Action>
    auto with_transition(std::string name, TargetState target, rxsc::scheduler::clock_type::duration duration, Action action)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value>::value, this_type&>::type
    {
        return with_transition(std::move(name), std::move(target), identity_current_thread(), std::move(duration), std::move(action));
    }

    /*!  \brief  Adds a timeout transition to another vertex using the identity_current_thread coordination with a specified guard.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param duration  The timeout duration.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Guard>
    auto with_transition(std::string name, TargetState target, rxsc::scheduler::clock_type::duration duration, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        return with_transition(std::move(name), std::move(target), identity_current_thread(), std::move(duration), std::move(guard));
    }

    /*!  \brief  Adds a timeout transition to another vertex using the identity_current_thread coordination with a specified action and guard.

         \tparam TargetState   The target vertex type (\a pseudostate or \a state).
         \tparam Action        The action callable type with signature void().
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param target    The target vertex.
         \param duration  The timeout duration.
         \param action    The action callable.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class TargetState, class Action, class Guard>
    auto with_transition(std::string name, TargetState target, rxsc::scheduler::clock_type::duration duration, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        return with_transition(std::move(name), std::move(target), identity_current_thread(), std::move(duration), std::move(action), std::move(guard));
    }

    /*!  \brief  Adds an internal triggered transition.

         \tparam Trigger  The trigger type, an observable.

         \param name     The name of the transition.
         \param trigger  The trigger observable.

         \return  A reference to self.
     */
    template<class Trigger>
    auto with_transition(std::string name, Trigger trigger)
        -> typename std::enable_if<rxu::all_true<is_observable<Trigger>::value>::value, state&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, std::move(trigger), detail::empty_action<Trigger>(), detail::empty_guard<Trigger>());
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal triggered transition with a specified action.

         \tparam Trigger  The trigger type, an observable.
         \tparam Action   The action callable type with signature void().

         \param name     The name of the transition.
         \param trigger  The trigger observable.
         \param action   The action callable.

         \return  A reference to self.
     */
    template<class Trigger, class Action>
    auto with_transition(std::string name, Trigger trigger, Action action)
        -> typename std::enable_if<rxu::all_true<is_observable<Trigger>::value,
                                                 is_action_of<rxu::value_type_t<Trigger>, Action>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, std::move(trigger), std::move(action), detail::empty_guard<Trigger>());
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal triggered transition with a specified guard.

         \tparam Trigger  The trigger type, an observable.
         \tparam Guard    The guard callable type with signature bool().

         \param name     The name of the transition.
         \param trigger  The trigger observable.
         \param guard    The guard callable.

         \return  A reference to self.
     */
    template<class Trigger, class Guard>
    auto with_transition(std::string name, Trigger trigger, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_observable<Trigger>::value,
                                                 is_guard_of<rxu::value_type_t<Trigger>, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, std::move(trigger), detail::empty_action<Trigger>(), std::move(guard));
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal triggered transition with a specified action and guard.

         \tparam Trigger  The trigger type, an observable.
         \tparam Action   The action callable type with signature void().
         \tparam Guard    The guard callable type with signature bool().

         \param name     The name of the transition.
         \param trigger  The trigger observable.
         \param action   The action callable.
         \param guard    The guard callable.

         \return  A reference to self.
     */
    template<class Trigger, class Action, class Guard>
    auto with_transition(std::string name, Trigger trigger, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_observable<Trigger>::value,
                                                 is_action_of<rxu::value_type_t<Trigger>, Action>::value,
                                                 is_guard_of<rxu::value_type_t<Trigger>, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, std::move(trigger), std::move(action), std::move(guard));
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal timeout transition of a specified coordination.

         \tparam Coordination  The type of coordination of which to perform the timeout.

         \param name      The name of the transition.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.

         \return  A reference to self.
     */
    template<class Coordination>
    auto with_transition(std::string name, Coordination cn, rxsc::scheduler::clock_type::duration duration)
        -> typename std::enable_if<is_coordination<Coordination>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, std::move(cn), std::move(duration), detail::empty_action<void>(), detail::empty_guard<void>());
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal timeout transition of a specified coordination with a specified action.

         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Action        The action callable type with signature void().

         \param name      The name of the transition.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param action    The action callable.

         \return  A reference to self.
     */
    template<class Coordination, class Action>
    auto with_transition(std::string name, Coordination cn, rxsc::scheduler::clock_type::duration duration, Action action)
        -> typename std::enable_if<rxu::all_true<is_coordination<Coordination>::value,
                                                 is_action_of<void, Action>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), delegate, std::move(cn), std::move(duration), std::move(action), detail::empty_guard<void>());
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal timeout transition of a specified coordination with a specified guard.

         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class Coordination, class Guard>
    auto with_transition(std::string name, Coordination cn, rxsc::scheduler::clock_type::duration duration, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_coordination<Coordination>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, std::move(cn), std::move(duration), detail::empty_action<void>(), std::move(guard));
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal timeout transition of a specified coordination with a specified action and guard.

         \tparam Coordination  The type of coordination of which to perform the timeout.
         \tparam Action        The action callable type with signature void().
         \tparam Guard         The guard callable type with signature bool().

         \param name      The name of the transition.
         \param cn        The coordination of which to perform the timeout.
         \param duration  The timeout duration.
         \param action    The action callable.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class Coordination, class Action, class Guard>
    auto with_transition(std::string name, Coordination cn, rxsc::scheduler::clock_type::duration duration, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_coordination<Coordination>::value,
                                                 is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), delegate, std::move(cn), std::move(duration), std::move(action), std::move(guard));
        delegate->check_transition_source(t);
        delegate->check_transition_target(t);
        delegate->add_transition(t);
        return *this;
    }

    /*!  \brief  Adds an internal timeout transition using the identity_current_thread coordination.

         \param name      The name of the transition.
         \param duration  The timeout duration.

         \return  A reference to self.
     */
    this_type& with_transition(std::string name, rxsc::scheduler::clock_type::duration duration)
    {
        return with_transition(std::move(name), identity_current_thread(), std::move(duration));
    }

    /*!  \brief  Adds an internal timeout transition using the identity_current_thread coordination with a specified action.

         \tparam Action  The action callable type with signature void().

         \param name      The name of the transition.
         \param duration  The timeout duration.
         \param action    The action callable.

         \return  A reference to self.
     */
    template<class Action>
    auto with_transition(std::string name, rxsc::scheduler::clock_type::duration duration, Action action)
        -> typename std::enable_if<is_action_of<void, Action>::value, this_type&>::type
    {
        return with_transition(std::move(name), identity_current_thread(), std::move(duration), std::move(action));
    }

    /*!  \brief  Adds an internal timeout transition using the identity_current_thread coordination with a specified guard.

         \tparam Guard  The guard callable type with signature bool().

         \param name      The name of the transition.
         \param duration  The timeout duration.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class Guard>
    auto with_transition(std::string name, rxsc::scheduler::clock_type::duration duration, Guard guard)
        -> typename std::enable_if<is_guard_of<void, Guard>::value, this_type&>::type
    {
        return with_transition(std::move(name), identity_current_thread(), std::move(duration), std::move(guard));
    }

    /*!  \brief  Adds an internal timeout transition using the identity_current_thread coordination with a specified action and guard.

         \tparam Action  The action callable type with signature void().
         \tparam Guard   The guard callable type with signature bool().

         \param name      The name of the transition.
         \param duration  The timeout duration.
         \param action    The action callable.
         \param guard     The guard callable.

         \return  A reference to self.
     */
    template<class Action, class Guard>
    auto with_transition(std::string name, rxsc::scheduler::clock_type::duration duration, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        return with_transition(std::move(name), identity_current_thread(), std::move(duration), std::move(action), std::move(guard));
    }

    /*!  \brief  Adds sub states to this state.

         If the state is simple, adding sub states to it implicitly makes it a composite state.

         \tparam SubState0  Type of the first sub state to add. Must be a vertex type (derived from \a state or \a pseudostate).
         \tparam SubStateN  Types of subsequent sub states to add, if any.

         \param sub_state0  The first sub state to add.
         \param sub_stateN  Subsequent sub states to add, if any.

         \return  A reference to self
     */
    template<class SubState0, class... SubStateN>
    auto with_sub_state(SubState0 sub_state0, SubStateN... sub_stateN)
        -> typename std::enable_if<is_vertices<SubState0, SubStateN...>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        if (delegate->type == detail::state_delegate::simple) {
            delegate->type = detail::state_delegate::composite;
            auto r = make_region("");
            WithRegion<region>()(this, std::move(r));
        }
        if (delegate->type != detail::state_delegate::composite) {
            delegate->throw_exception<not_allowed>("is not a composite state and is not allowed to have sub states");
        }
        auto r = region(std::static_pointer_cast<detail::region_delegate>(delegate->regions.front()));
        r.with_sub_state(sub_state0, sub_stateN...);
        return *this;
    }

    /*!  \brief  Adds orthogonal regions to this state.

         If the state is simple, adding regions to it implicitly makes it an orthogonal state.

         \tparam Region0  Type of the first region to add. Must be derived from \a region.
         \tparam RegionN  Types of subsequent regions to add, if any.

         \param region0  The first region to add.
         \param regionN  Subsequent regions to add, if any.

         \return  A reference to self
     */
    template<class Region0, class... RegionN>
    auto with_region(Region0 region0, RegionN... regionN)
        -> typename std::enable_if<is_regions<Region0, RegionN...>::value, this_type&>::type
    {
        if (delegate->is_assembled()) {
            delegate->throw_exception<not_allowed>("state machine already assembled");
        }
        if (delegate->type == detail::state_delegate::simple) {
            delegate->type = detail::state_delegate::orthogonal;
        }
        if (delegate->type != detail::state_delegate::orthogonal) {
            delegate->throw_exception<not_allowed>("is not an orthogonal state and is not allowed to have regions");
        }
        return WithRegion<Region0, RegionN...>()(this, std::move(region0), std::move(regionN)...);
    }

    /*!  \brief  Adds a sub state machine to this state.

         If the state is simple, adding a state machine implicitly makes it a sub machine state.

         \tparam StateMachine  Type of the state machine to add. Must be derived from \a state_machine.

         \param state_machine  The state machine to add.

         \return  A reference to self
     */
    template<class StateMachine>
    inline auto with_state_machine(StateMachine state_machine)
        -> typename std::enable_if<is_state_machine<StateMachine>::value, this_type&>::type;
};

/*!  \brief  A final state is a special kind of state signifying that the enclosing region has completed.

     A transition to a final state represents the completion of the behaviors of the region containing the final state.

     \image html final_state.png

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class final_state : public detail::virtual_vertex<detail::final_state_delegate>
                  , public detail::state_base
{
private:

    final_state() = delete;

    explicit final_state(std::shared_ptr<delegate_type> d);

    explicit final_state(std::string n);

    friend class state_machine;
    friend class transition;
    friend final_state make_final_state(std::string);
    RX_FSM_FRIEND_OPERATORS(final_state)

public:

    typedef final_state this_type;
    typedef detail::state_delegate::state_t state_t;
};

/*!  \brief Creates a state.

     \param name  The name of the state.

     \return  A \a state instance.
 */
state make_state(std::string name);

/*!  \brief Creates a final state.

     \param name  The name of the final state.

     \return  A \a final_state instance.
 */
final_state make_final_state(std::string name);

RX_FSM_OPERATORS(state)
RX_FSM_OPERATORS(final_state)

template<class State>
auto transition::source_state()
    -> typename std::enable_if<std::is_same<State, state>::value, State>::type
{
    auto src = source();
    auto s = std::dynamic_pointer_cast<detail::state_delegate>(src);
    if (!s) {
        std::ostringstream msg;
        msg << "source state '" << src->name << "' is not a regular state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return state(s);

}

template<class PseudoState>
auto transition::source_state()
    -> typename std::enable_if<std::is_same<PseudoState, pseudostate>::value, PseudoState>::type
{
    auto src = source();
    auto s = std::dynamic_pointer_cast<detail::pseudostate_delegate>(src);
    if (!s) {
        std::ostringstream msg;
        msg << "source state '" << src->name << "' is not a pseudo state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return pseudostate(s);
}

template<class FinalState>
auto transition::source_state()
    -> typename std::enable_if<std::is_same<FinalState, final_state>::value, FinalState>::type
{
    auto src = source();
    auto s = std::dynamic_pointer_cast<detail::final_state_delegate>(src);
    if (!s) {
        std::ostringstream msg;
        msg << "source state '" << src->name << "' is not a final state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return final_state(s);
}

template<class State>
auto transition::target_state()
    -> typename std::enable_if<std::is_same<State, state>::value, State>::type
{
    auto tgt = target();
    auto s = std::dynamic_pointer_cast<detail::state_delegate>(tgt);
    if (!s) {
        std::ostringstream msg;
        msg << "target state '" << tgt->name << "' is not a regular state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return state(s);
}

template<class PseudoState>
auto transition::target_state()
    -> typename std::enable_if<std::is_same<PseudoState, pseudostate>::value, PseudoState>::type
{
    auto tgt = target();
    auto s = std::dynamic_pointer_cast<detail::pseudostate_delegate>(tgt);
    if (!s) {
        std::ostringstream msg;
        msg << "target state '" << tgt->name << "' is not a pseudo state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return pseudostate(s);
}

template<class FinalState>
auto transition::target_state()
    -> typename std::enable_if<std::is_same<FinalState, final_state>::value, FinalState>::type
{
    auto tgt = target();
    auto s = std::dynamic_pointer_cast<detail::final_state_delegate>(tgt);
    if (!s) {
        std::ostringstream msg;
        msg << "target state '" << tgt->name << "' is not a final state";
        delegate->throw_exception<state_error>(msg.str());
    }
    return final_state(s);
}

}
}

RX_FSM_HASH(rxcpp::fsm::state)
RX_FSM_HASH(rxcpp::fsm::final_state)

#endif
