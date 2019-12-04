/*! \file  rx-fsm-transition.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-transition.hpp"
#include "rxcpp/fsm/rx-fsm-pseudostate.hpp"
#include "rxcpp/fsm/rx-fsm-state.hpp"
#include "rxcpp/fsm/rx-fsm-state_machine.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

std::shared_ptr<virtual_vertex_delegate> transition_delegate::target() const
{
    return target_.lock();
}

std::string transition_delegate::type_name() const
{
    return "transition";
}

void transition_delegate::guard_executed() const
{
    state_machine()->guard_executed(owner<virtual_vertex_delegate>());
}

transition_delegate::transition_delegate(bool g, const std::shared_ptr<virtual_vertex_delegate>& tgt, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, transition_t t)
    : element_delegate(std::move(n), o)
    , guarded(g)
    , target_(tgt)
    , type(t)
    , blocked(false)
{
}

transition_delegate::transition_delegate(bool g, std::string n, const std::shared_ptr<virtual_vertex_delegate>& o, transition_t t)
    : element_delegate(std::move(n), o)
    , guarded(g)
    , type(t)
    , blocked(false)
{
}

std::vector<std::shared_ptr<transition_delegate>> type_filtered_transitions(transition_delegate::transition_t type, const std::vector<std::shared_ptr<transition_delegate>>& transitions)
{
    std::vector<std::shared_ptr<transition_delegate>> result;
    for(const auto& t : transitions)
    {
        if (t->type == type) {
            result.push_back(t);
        }
    }
    return result;
}

observable<completion_transition_value_type> create_completion_trigger(const std::weak_ptr<triggered_transition_delegate<observable<completion_transition_value_type>>>& transition)
{
    auto factory = [transition]() {
        auto t = transition.lock();
        if (t) {
            if (!t->blocked.load()) {
                return observable<>::just(completion_transition_value_type()).as_dynamic();
            } else {
                return t->subject.get_observable() ;
            }
        }
        return observable<>::never<completion_transition_value_type>().as_dynamic();
    };
    return observable<>::defer(factory);
}

}

transition::transition(std::shared_ptr<delegate_type> d)
        : element<delegate_type>(std::move(d))
{
}

std::shared_ptr<detail::virtual_vertex_delegate> transition::source() const
{
    auto s = delegate->owner<detail::virtual_vertex_delegate>();
    if (!s) {
        delegate->throw_exception<deleted_error>("state machine is deleted");
    }
    return s;
}

std::shared_ptr<detail::virtual_vertex_delegate> transition::target() const
{
    auto s = delegate->target();
    if (!s) {
        delegate->throw_exception<deleted_error>("state machine is deleted");
    }
    return s;
}

transition::state_type transition::source_state_type() const
{
    auto src = source();
    auto& raw = *src.get();
    if (typeid(raw) == typeid(detail::pseudostate_delegate)) {
        return pseudo;
    } else if (typeid(raw) == typeid(detail::state_delegate)) {
        return regular;
    } else {
        return final;
    }
}

transition::state_type transition::target_state_type() const
{
    auto tgt = target();
    auto &raw = *tgt.get();
    if (typeid(raw) == typeid(detail::pseudostate_delegate)) {
        return pseudo;
    } else if (typeid(raw) == typeid(detail::state_delegate)) {
        return regular;
    } else {
        return final;
    }
}

}
}
