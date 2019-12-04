/*! \file  rx-fsm-state.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-state.hpp"
#include "rxcpp/fsm/rx-fsm-state_machine.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

void state_delegate::check_transition_target(const std::shared_ptr<transition_delegate>&) const
{
}

void state_delegate::check_transition_source(const std::shared_ptr<transition_delegate>&) const
{
}

void state_delegate::validate_sub_states(const std::shared_ptr<virtual_region_delegate>& region)
{
    auto state = region->owner<state_delegate>();
    std::shared_ptr<element_delegate> thrower;
    if (state->type == state_delegate::orthogonal) {
        thrower = region;
    } else {
        thrower = state;
    }
    bool initial(false), deep_history(false), shallow_history(false), states(false);
    for (const auto& sub_state : region->sub_states)
    {
        auto pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(sub_state);
        if (pseudostate) {
            switch (pseudostate->type) {
            case pseudostate_kind::initial:
                if (initial) {
                    thrower->throw_exception<not_allowed>("cannot have multiple initial pseudostate sub states");
                }
                initial = true;
                break;
            case pseudostate_kind::deep_history:
                if (deep_history) {
                    thrower->throw_exception<not_allowed>("cannot have multiple deep history pseudostate sub states");
                }
                deep_history = true;
                break;
            case pseudostate_kind::shallow_history:
                if (shallow_history) {
                    thrower->throw_exception<not_allowed>("cannot have multiple shallow history pseudostate sub states");
                }
                shallow_history = true;
                break;
            default:
                break;
            }
            continue;
        }
        auto s = std::dynamic_pointer_cast<state_delegate>(sub_state);
        if (s) {
            states = true;
        }
    }
    if (!initial) {
        auto sm = state->state_machine();
        auto it = sm->target_states.find(state);
        if (it != sm->target_states.end()) {
           thrower->throw_exception<not_allowed>("must have an initial pseudostate");
        }
    }
    if (!states) {
        thrower->throw_exception<not_allowed>("must have regular states");
    }
}

void state_delegate::validate() const
{
    virtual_vertex_delegate::validate();
    switch(type) {
    case composite:
    case orthogonal:
    {
        for(const auto& region : regions)
        {
            validate_sub_states(region);
        }
        break;
    }
    default:
        ;
    }
    for(const auto& t : transitions)
    {
        auto target_state = t->target();
        if (target_state) {
            if (state_machine() != target_state->state_machine()) {
                std::ostringstream msg;
                msg << "transitions to '" << target_state->name << "' which is not of the same state machine";
                throw_exception<not_allowed>(msg.str());
            }
        }
    }
    for(const auto& region : regions)
    {
        for(const auto& sub_state : region->sub_states)
        {
            sub_state->validate();
        }
    }
}

void state_delegate::on_entry()
{
    if (entry) {
        entry();
    }
}

void state_delegate::on_exit()
{
   if (exit) {
       exit();
   }
}

bool state_delegate::contains(const std::shared_ptr<virtual_region_delegate>& region) const
{
    for(const auto& r : regions)
    {
        if (r == region) {
            return true;
        }
        for(const auto& state : r->sub_states)
        {
            auto s = std::dynamic_pointer_cast<state_delegate>(state);
            if (s) {
                if (s->contains(region)) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::string state_delegate::type_name() const
{
   switch(type) {
   case composite:
       return "composite state";
   case orthogonal:
       return "orthogonal state";
   case sub_machine:
       return "sub machine state";
   default:
       ;
   }
   return "state";
}

state_delegate::state_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : virtual_vertex_delegate(std::move(n), o)
    , type(simple)
{
}

state_delegate::state_delegate(std::string n)
    : virtual_vertex_delegate(std::move(n))
    , type(simple)
{
}

void final_state_delegate::check_transition_target(const std::shared_ptr<transition_delegate>&) const
{
}

void final_state_delegate::check_transition_source(const std::shared_ptr<transition_delegate>&) const
{
}

void final_state_delegate::validate() const
{
}

std::string final_state_delegate::type_name() const
{
    return "final state";
}

final_state_delegate::final_state_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : virtual_vertex_delegate(std::move(n), o)
{
}

final_state_delegate::final_state_delegate(std::string n)
    : virtual_vertex_delegate(std::move(n))
{
}

}

state::state(std::shared_ptr<delegate_type> d)
    : detail::virtual_vertex<delegate_type>(std::move(d))
{
}

state::state(std::string n)
    : detail::virtual_vertex<delegate_type>(std::make_shared<delegate_type>(std::move(n)))
{
}

bool state::check_region(const std::shared_ptr<detail::virtual_region_delegate>& region)
{
    const auto& regions = delegate->regions;
    auto it = std::find_if(regions.begin(), regions.end(), [&region](const std::shared_ptr<detail::virtual_region_delegate>& r) {
        return r == region || r->name == region->name;
    });
    return it == regions.end();
}

bool state::is_simple() const
{
    return delegate->type == detail::state_delegate::simple;
}

bool state::is_composite() const
{
    return delegate->type == detail::state_delegate::composite;
}

bool state::is_orthogonal() const
{
    return delegate->type == detail::state_delegate::orthogonal;
}

bool state::is_sub_machine() const
{
    return delegate->type == detail::state_delegate::sub_machine;
}

final_state::final_state(std::shared_ptr<delegate_type> d)
    : detail::virtual_vertex<delegate_type>(std::move(d))
{
}

final_state::final_state(std::string n)
    : detail::virtual_vertex<delegate_type>(std::make_shared<delegate_type>(std::move(n)))
{
}

state make_state(std::string name)
{
    return state(std::move(name));
}

final_state make_final_state(std::string name)
{
    return final_state(std::move(name));
}

}
}
