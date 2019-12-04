/*! \file  rx-fsm-vertex.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_VERTEX_HPP)
#define RX_FSM_VERTEX_HPP

#include "rx-fsm-transition.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

template<class Delegate>
class virtual_vertex : public element<Delegate>
{
public:

    typedef virtual_vertex<Delegate> this_type;

    virtual ~virtual_vertex() = default;

protected:

    virtual_vertex() = default;

    virtual_vertex(std::shared_ptr<Delegate> d)
        : element<Delegate>(std::move(d))
    {
    }

    // completion transitions

    template<class TargetState>
    auto with_transition(std::string name, TargetState target)
        -> typename std::enable_if<is_vertex<TargetState>::value, virtual_vertex&>::type
    {
        if (this->delegate->is_assembled()) {
            this->delegate->template throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), this->delegate, target, detail::empty_action<void>(), detail::empty_guard<void>());
        this->delegate->check_transition_source(t);
        target()->check_transition_target(t);
        this->delegate->add_transition(t);
        return *this;
    }

    template<class TargetState, class Action>
    auto with_transition(std::string name, TargetState target, Action action)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value>::value, this_type&>::type
    {
        if (this->delegate->is_assembled()) {
            this->delegate->template throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(false, std::move(name), this->delegate, target, std::move(action), detail::empty_guard<void>());
        this->delegate->check_transition_source(t);
        target()->check_transition_target(t);
        this->delegate->add_transition(t);
        return *this;
    }

    template<class TargetState, class Guard>
    auto with_transition(std::string name, TargetState target, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (this->delegate->is_assembled()) {
            this->delegate->template throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), this->delegate, target, detail::empty_action<void>(), std::move(guard));
        this->delegate->check_transition_source(t);
        target()->check_transition_target(t);
        this->delegate->add_transition(t);
        return *this;
    }

    template<class TargetState, class Action, class Guard>
    auto with_transition(std::string name, TargetState target, Action action, Guard guard)
        -> typename std::enable_if<rxu::all_true<is_vertex<TargetState>::value,
                                                 is_action_of<void, Action>::value,
                                                 is_guard_of<void, Guard>::value>::value, this_type&>::type
    {
        if (this->delegate->is_assembled()) {
            this->delegate->template throw_exception<not_allowed>("state machine already assembled");
        }
        auto t = detail::make_transition(true, std::move(name), this->delegate, target, std::move(action), std::move(guard));
        this->delegate->check_transition_source(t);
        target()->check_transition_target(t);
        this->delegate->add_transition(t);
        return *this;
    }

};

}
}
}

#endif
