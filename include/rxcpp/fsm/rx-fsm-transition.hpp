/*! \file  rx-fsm-transition.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_TRANSITION_HPP)
#define RX_FSM_TRANSITION_HPP

#include <atomic>

#include "rx-fsm-delegates.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

struct transition_delegate : public element_delegate
                           , public std::enable_shared_from_this<transition_delegate>
{
    typedef transition_delegate this_type;

    enum transition_t
    {
        completion,
        triggered,
        timeout
    };

    bool guarded;

    std::weak_ptr<virtual_vertex_delegate> target_;

    transition_t type;

    std::atomic<bool> blocked;

    std::shared_ptr<virtual_vertex_delegate> target() const;

    virtual bool try_block() = 0;
    virtual void unblock() = 0;
    virtual bool equal_trigger(const std::shared_ptr<transition_delegate>& other) const = 0;

    struct action
    {
        virtual void execute() = 0;
        virtual ~action() = default;
    };

    virtual void execute_action() = 0;

    typedef std::function<void()> void_action_t;
    typedef std::function<bool()> void_guard_t;

    typedef std::tuple<std::shared_ptr<virtual_vertex_delegate>, std::shared_ptr<transition_delegate>, std::shared_ptr<action>> transition_data;

    virtual observable<transition_data> make_observable(const std::vector<std::shared_ptr<transition_delegate>>& transitions) = 0;

    virtual std::string type_name() const override;

    void guard_executed() const;

    explicit transition_delegate(bool g, const std::shared_ptr<virtual_vertex_delegate>& tgt, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, transition_t t);

    explicit transition_delegate(bool g, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, transition_t t);

    transition_delegate() = default;

    virtual ~transition_delegate() override = default;
};

struct completion_transition_value_type {};

template<class Trigger>
struct triggered_transition_delegate : public transition_delegate
{
    static const bool is_trigger_equality_comparable = is_equality_comparable<typename Trigger::source_operator_type>::value;

    typedef triggered_transition_delegate<Trigger> this_type;
    typedef rxu::value_type_t<Trigger> value_type;

    Trigger trigger;

    composite_subscription subject_lifetime;
    rxsub::subject<value_type> subject;

    virtual bool try_block() override
    {
        bool expected(false);
        return blocked.compare_exchange_strong(expected, true);
    }

    template<class T = value_type>
    void typed_unblock(typename std::enable_if<std::is_same<completion_transition_value_type, T>::value>::type* = 0)
    {
        if (subject.get_subscriber().is_subscribed()) {
            subject.get_subscriber().on_next(completion_transition_value_type());
            subject.get_subscriber().on_completed();
        }
    }

    template<class T = value_type>
    void typed_unblock(typename std::enable_if<!std::is_same<completion_transition_value_type, T>::value>::type* = 0)
    {
    }

    virtual void unblock() override
    {
        bool expected(true);
        if (blocked.compare_exchange_strong(expected, false)) {
            typed_unblock();
        }
    }

    virtual bool equal_trigger(const std::shared_ptr<transition_delegate>& other) const override
    {
        if (!is_trigger_equality_comparable) {
            return false;
        }
        this_type* d = dynamic_cast<this_type*>(other.get());
        return d && (d->trigger == this->trigger);
    }

    template<class T = value_type>
    void typed_execute_action(typename std::enable_if<std::is_same<completion_transition_value_type, T>::value>::type* = 0)
    {
       action(completion_transition_value_type());
    }

    template<class T = value_type>
    void typed_execute_action(typename std::enable_if<!std::is_same<completion_transition_value_type, T>::value>::type* = 0)
    {
    }

    virtual void execute_action() override
    {
        typed_execute_action();
    }

    typedef std::function<void(const value_type&)> action_t;
    typedef std::function<bool(const value_type&)> guard_t;

    struct typed_action : public action
    {
        action_t action;
        value_type value;

        explicit typed_action(action_t a, value_type v)
        : action(std::move(a))
        , value(std::move(v))
        {
        }

        virtual void execute() override
        {
            action(value);
        }
    };

    action_t action;
    guard_t guard;

    struct scoped_transition_block
    {
        std::vector<std::shared_ptr<transition_delegate>> transitions;

        explicit scoped_transition_block(std::vector<std::shared_ptr<transition_delegate>> t)
        : transitions(std::move(t))
        {
        }

        ~scoped_transition_block()
        {
            for(const auto& t_ : transitions)
            {
                t_->unblock();
            }
        }
    };

    typedef resource<std::shared_ptr<scoped_transition_block>> transition_resource;

    virtual observable<transition_data> make_observable(const std::vector<std::shared_ptr<transition_delegate>>& transitions) override
    {
        std::vector<std::shared_ptr<transition_delegate>> ancestor_transitions;
        auto state = this->owner<virtual_vertex_delegate>();
        std::copy_if(transitions.begin(), transitions.end(), std::back_inserter(ancestor_transitions), [&state](const std::shared_ptr<transition_delegate>& t) {
            auto s = t->owner<virtual_vertex_delegate>();
            return s != state;
        });
        auto resource_factory = [ancestor_transitions]() {
            std::vector<std::shared_ptr<transition_delegate>> blocked;
            for(const auto& t : ancestor_transitions)
            {
                if (t->try_block())
                {
                    blocked.push_back(t);
                }
            }
            return transition_resource(std::make_shared<scoped_transition_block>(blocked));
        };
        auto self = std::static_pointer_cast<this_type>(this->shared_from_this());
        auto observable_factory = [self, transitions](const transition_resource&) {
            return self->trigger.map([transitions](const value_type& v) -> transition_data {
                auto self = transitions.front();
                if (self->blocked.load()) {
                    return std::make_tuple(std::shared_ptr<virtual_vertex_delegate>(), std::shared_ptr<transition_delegate>(), std::shared_ptr<transition_delegate::action>());
                }
                for(const auto& t : transitions)
                {
                    auto tt = std::static_pointer_cast<this_type>(t);
                    t->guard_executed();
                    if (tt->guard(v)) {
                        auto source = self->owner<virtual_vertex_delegate>();
                        return std::make_tuple(source, t, std::make_shared<typed_action>(tt->action, v));
                    }
                }
                return std::make_tuple(std::shared_ptr<virtual_vertex_delegate>(), std::shared_ptr<transition_delegate>(), std::shared_ptr<transition_delegate::action>());
            }).filter([](const transition_data& data) {
                return std::get<0>(data);
            });
        };
        return observable<>::scope(resource_factory, observable_factory);
    }

    explicit triggered_transition_delegate(bool guarded_, const std::shared_ptr<virtual_vertex_delegate>& tgt, Trigger trig, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, action_t a, guard_t g, transition_t type_)
        : transition_delegate(guarded_, tgt, std::move(n), o, type_)
        , trigger(std::move(trig))
        , subject(subject_lifetime)
        , action(std::move(a))
        , guard(std::move(g))
    {
    }

    explicit triggered_transition_delegate(bool guarded_, Trigger trig, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, action_t a, guard_t g, transition_t type_)
        : transition_delegate(guarded_, std::move(n), o, type_)
        , trigger(std::move(trig))
        , subject(subject_lifetime)
        , action(std::move(a))
        , guard(std::move(g))
    {
    }

    explicit triggered_transition_delegate(bool guarded_, const std::shared_ptr<virtual_vertex_delegate>& tgt, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, action_t a, guard_t g, transition_t type_)
        : transition_delegate(guarded_, tgt, std::move(n), o, type_)
        , subject(subject_lifetime)
        , action(std::move(a))
        , guard(std::move(g))
    {
    }

};

std::vector<std::shared_ptr<transition_delegate>> type_filtered_transitions(transition_delegate::transition_t type, const std::vector<std::shared_ptr<transition_delegate>>& transitions);

template<class Coordination>
observable<long> create_timeout_trigger(Coordination cn, rxsc::scheduler::clock_type::duration dur)
{
    auto factory = [cn, dur]() {
        return observable<>::timer(dur, cn);
    };
    return observable<>::defer(factory);
}

observable<completion_transition_value_type> create_completion_trigger(const std::weak_ptr<triggered_transition_delegate<observable<completion_transition_value_type>>>& transition);

template<class TargetState, class Trigger>
typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                             is_observable<Trigger>::value>::value, std::shared_ptr<transition_delegate>>::type
    make_transition(bool guarded, std::string name, const std::shared_ptr<virtual_vertex_delegate>& owner, const TargetState& target, Trigger trigger, typename triggered_transition_delegate<Trigger>::action_t action, typename triggered_transition_delegate<Trigger>::guard_t guard)
{
    std::shared_ptr<virtual_vertex_delegate> tgt = std::dynamic_pointer_cast<virtual_vertex_delegate>(target());
    return std::make_shared<triggered_transition_delegate<Trigger>>(guarded, tgt, std::move(trigger), std::move(name), owner, std::move(action), std::move(guard), detail::transition_delegate::triggered);
}

template<class Trigger>
typename std::enable_if<is_observable<Trigger>::value, std::shared_ptr<transition_delegate>>::type
    make_transition(bool guarded, std::string name, const std::shared_ptr<virtual_vertex_delegate>& owner, Trigger trigger, typename triggered_transition_delegate<Trigger>::action_t action, typename triggered_transition_delegate<Trigger>::guard_t guard)
{
    return std::make_shared<triggered_transition_delegate<Trigger>>(guarded, std::move(trigger), std::move(name), owner, std::move(action), std::move(guard), detail::transition_delegate::triggered);
}

template<class Coordination, class TargetState>
typename std::enable_if<rxu::all_true<is_coordination<Coordination>::value,
                                             is_vertex<TargetState>::value>::value, std::shared_ptr<transition_delegate>>::type
    make_transition(bool guarded, std::string name, const std::shared_ptr<virtual_vertex_delegate>& owner, const TargetState& target, Coordination cn, rxsc::scheduler::clock_type::duration dur, transition_delegate::void_action_t action, transition_delegate::void_guard_t guard)
{
    typedef typename std::result_of<decltype(&create_timeout_trigger<Coordination>)(Coordination, rxsc::scheduler::clock_type::duration)>::type observable_type;
    typedef typename triggered_transition_delegate<observable_type>::value_type value_type;
    typedef typename triggered_transition_delegate<observable_type>::action_t action_t;
    typedef typename triggered_transition_delegate<observable_type>::guard_t guard_t;
    action_t a = [action](const value_type&) {
        action();
    };
    guard_t g = [guard](const value_type&) {
        return guard();
    };
    std::shared_ptr<virtual_vertex_delegate> tgt = std::dynamic_pointer_cast<virtual_vertex_delegate>(target());
    auto trigger = create_timeout_trigger<Coordination>(std::move(cn), dur);
    return std::make_shared<triggered_transition_delegate<observable_type>>(guarded, tgt, std::move(trigger), std::move(name), owner, std::move(a), std::move(g), detail::transition_delegate::timeout);
}

template<class Coordination>
typename std::enable_if<is_coordination<Coordination>::value, std::shared_ptr<transition_delegate>>::type
    make_transition(bool guarded, std::string name, const std::shared_ptr<virtual_vertex_delegate>& owner, Coordination cn, rxsc::scheduler::clock_type::duration dur, transition_delegate::void_action_t action, transition_delegate::void_guard_t guard)
{
    typedef typename std::result_of<decltype(&create_timeout_trigger<Coordination>)(Coordination, rxsc::scheduler::clock_type::duration)>::type observable_type;
    typedef typename triggered_transition_delegate<observable_type>::value_type value_type;
    typedef typename triggered_transition_delegate<observable_type>::action_t action_t;
    typedef typename triggered_transition_delegate<observable_type>::guard_t guard_t;
    action_t a = [action](const value_type&) {
        action();
    };
    guard_t g = [guard](const value_type&) {
        return guard();
    };
    auto trigger = create_timeout_trigger<Coordination>(std::move(cn), dur);
    return std::make_shared<triggered_transition_delegate<observable_type>>(guarded, std::move(trigger), std::move(name), owner, std::move(a), std::move(g), detail::transition_delegate::timeout);
}

template<class TargetState>
typename std::enable_if<is_vertex<TargetState>::value, std::shared_ptr<transition_delegate>>::type
    make_transition(bool guarded, std::string name, const std::shared_ptr<virtual_vertex_delegate>& owner, const TargetState& target, transition_delegate::void_action_t action, transition_delegate::void_guard_t guard)
{
    typedef observable<completion_transition_value_type> observable_type;
    typedef completion_transition_value_type value_type;
    typedef typename triggered_transition_delegate<observable_type>::action_t action_t;
    typedef typename triggered_transition_delegate<observable_type>::guard_t guard_t;
    action_t a = [action](const value_type&) {
        action();
    };
    guard_t g = [guard](const value_type&) {
        return guard();
    };
    std::shared_ptr<virtual_vertex_delegate> tgt = std::dynamic_pointer_cast<virtual_vertex_delegate>(target());
    auto t = std::make_shared<triggered_transition_delegate<observable_type>>(guarded, tgt, std::move(name), owner, std::move(a), std::move(g), detail::transition_delegate::completion);
    t->trigger = create_completion_trigger(t);
    return t;
}

}

class final_state;
class pseudostate;
class state;

/*!  \brief  A Transition is a single directed arc originating from a single source vertex and terminating on a single target vertex (the
             source and target may be the same vertex), which specifies a valid fragment of a state machine behavior.

      It may have an associated effect behavior, which is executed when the transition is traversed (executed).

      \note The duration of a transition traversal is undefined, allowing for different semantic interpretations, including
            both “zero” and non-“zero” time.

      Transitions are executed as part of a more complex compound transition that takes a state machine execution from one
      stable state configuration to another. The semantics of compound transitions are described below.
      In the course of execution, a Transition instance is said to be:
        - reached, when execution of its state machine execution has reached its source vertex;
        - traversed, when it is being executed (along with any associated effect behavior); and
        - completed, after it has reached its target vertex.

      A transition may own a set of triggers, each of which specifies an event whose occurrence, when dispatched, may
      trigger traversal of the transition. A transition trigger is said to be enabled if the dispatched event occurrence matches
      its event type. When multiple triggers are defined for a Transition, they are logically disjunctive, that is, if any of them
      are enabled, the transition will be triggered.

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class transition final : public element<detail::transition_delegate>
{
public:

    typedef transition this_type;

private:

    transition() = delete;

    explicit transition(std::shared_ptr<delegate_type> d);

    std::shared_ptr<detail::virtual_vertex_delegate> source() const;

    std::shared_ptr<detail::virtual_vertex_delegate> target() const;
    
    friend struct detail::state_machine_delegate;
    RX_FSM_FRIEND_OPERATORS(transition)

public:
    
    /*!  \brief Type of originating state.
     */
    enum state_type
    {
        pseudo,
        regular,
        final
    };

    /*!  \return  The \a state_type of the source state.
     */
    state_type source_state_type() const;
    
    /*!  \return  The \a state_type of the target state.
     */
    state_type target_state_type() const;
    
    /*!  \brief  Getter for the transition's source state.

         \tparam State  The state type, i.e. \a state

         \return  An instance of \a state.
     */
    template<class State>
    inline auto source_state()
        -> typename std::enable_if<std::is_same<State, state>::value, State>::type;

    /*!  \brief  Getter for the transition's source state.

         \tparam PseodoState  The state type, i.e. \a pseudostate

         \return  An instance of \a pseudostate.
     */
    template<class PseudoState>
    inline auto source_state()
        -> typename std::enable_if<std::is_same<PseudoState, pseudostate>::value, PseudoState>::type;

    /*!  \brief  Getter for the transition's source state.

         \tparam FinalState  The state type, i.e. \a final_state

         \return  An instance of \a final_state.
     */
    template<class FinalState>
    inline auto source_state()
        -> typename std::enable_if<std::is_same<FinalState, final_state>::value, FinalState>::type;

    /*!  \brief  Getter for the transition's target state.

         \tparam State  The state type, i.e. \a state

         \return  An instance of \a state.
     */
    template<class State>
    inline auto target_state()
        -> typename std::enable_if<std::is_same<State, state>::value, State>::type;

    /*!  \brief  Getter for the transition's target state.

         \tparam PseodoState  The state type, i.e. \a pseudostate

         \return  An instance of \a pseudostate.
     */
    template<class PseudoState>
    inline auto target_state()
        -> typename std::enable_if<std::is_same<PseudoState, pseudostate>::value, PseudoState>::type;

    /*!  \brief  Getter for the transition's target state.

         \tparam FinalState  The state type, i.e. \a final_state

         \return  An instance of \a final_state.
     */
    template<class FinalState>
    inline auto target_state()
        -> typename std::enable_if<std::is_same<FinalState, final_state>::value, FinalState>::type;
};

RX_FSM_OPERATORS(transition)

}
}

RX_FSM_HASH(rxcpp::fsm::transition)

#endif
