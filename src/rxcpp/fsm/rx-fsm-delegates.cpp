/*! \file  rx-fsm-delegates.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-delegates.hpp"
#include "rxcpp/fsm/rx-fsm-pseudostate.hpp"
#include "rxcpp/fsm/rx-fsm-state.hpp"
#include "rxcpp/fsm/rx-fsm-transition.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

bool virtual_region_delegate::contains(const std::shared_ptr<virtual_vertex_delegate>& sub_state) const
{
    for(const auto& state : sub_states)
    {
        if (state == sub_state) {
            return true;
        }
        auto s = std::dynamic_pointer_cast<state_delegate>(state);
        if (s) {
            for(const auto& region : s->regions)
            {
                if (region->contains(sub_state)) {
                    return true;
                }
            }
        }
    }
    return false;
}

virtual_region_delegate::virtual_region_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : element_delegate(std::move(n), o)
{
}

virtual_region_delegate::virtual_region_delegate(std::string n)
    : element_delegate(std::move(n))
{
}


void virtual_vertex_delegate::add_transition(const std::shared_ptr<transition_delegate>& t)
{
    auto it = std::find_if(transitions.begin(), transitions.end(), [&t](const std::shared_ptr<transition_delegate>& t_) {
        return t_->name == t->name;
    });
    if (it != transitions.end()) {
        throw_exception<not_allowed>("transition with the same name already exists");
    }
    transitions.push_back(t);
}

void virtual_vertex_delegate::validate() const
{
    for(const auto& t : transitions)
    {
        auto target_state = t->target();
        if (target_state) {
            auto region = target_state->owner<virtual_region_delegate>();
            if (region) {
                auto s = region->owner<state_delegate>();
                if (s && s->type == state_delegate::orthogonal) {
                    // target is sub state of orthogonal state
                    if (!is_owned_by<virtual_region_delegate>(region)) {
                        const auto* pseudostate = dynamic_cast<const pseudostate_delegate*>(this);
                        if (!pseudostate || pseudostate->type != pseudostate_kind::fork) {
                            throw_exception<not_allowed>("is a sub state of an orthogonal state and cannot be transitioned to from a state out of its enclosing region by other than a transition from a 'fork' pseudostate");
                        }
                    }
                }
            }
        }
    }
}

virtual_vertex_delegate::virtual_vertex_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : element_delegate(std::move(n), o)
{
}

virtual_vertex_delegate::virtual_vertex_delegate(std::string n)
    : element_delegate(std::move(n))
{
}

}
}
}
