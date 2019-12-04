/*! \file  rx-fsm-pseudostate.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include <unordered_map>

#include "rxcpp/fsm/rx-fsm-pseudostate.hpp"
#include "rxcpp/fsm/rx-fsm-state.hpp"
#include "rxcpp/fsm/rx-fsm-transition.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

void pseudostate_delegate::check_transition_target(const std::shared_ptr<transition_delegate>& t) const
{
    switch(type) {
    case pseudostate_kind::initial:
        throw_exception<not_allowed>("cannot have incoming transitions");
        break;
    case pseudostate_kind::join:
        if (t->guarded) {
            throw_exception<not_allowed>("cannot have guarded incoming transitions");
        }
        break;
    default:
        ;
    }
}

void pseudostate_delegate::check_transition_source(const std::shared_ptr<transition_delegate>& t) const
{
    switch(type) {
    case pseudostate_kind::terminate:
        throw_exception<not_allowed>("cannot have outgoing transitions");
        break;
    case pseudostate_kind::fork:
    case pseudostate_kind::deep_history:
    case pseudostate_kind::shallow_history:
    case pseudostate_kind::initial:
    case pseudostate_kind::entry_point:
    case pseudostate_kind::exit_point:
        if (t->guarded) {
            throw_exception<not_allowed>("cannot have guarded outgoing transitions");
        }
        if (type != pseudostate_kind::fork && !transitions.empty()) {
            throw_exception<not_allowed>("can have at most one outgoing transition");
        }
        if (type == pseudostate_kind::initial) {
            auto pseudostate = std::dynamic_pointer_cast<this_type>(t->target());
            if (pseudostate) {
                throw_exception<not_allowed>("must not transition to a pseudostate");
            }
        }
        break;
    default:
        ;
    }
}

bool pseudostate_delegate::is_orthogonal_state(const std::shared_ptr<virtual_region_delegate>& region)
{
    auto s = region->owner<state_delegate>();
    return s && s->type == state_delegate::orthogonal;
}

std::vector<std::shared_ptr<virtual_region_delegate>> pseudostate_delegate::orthogonal_regions(const std::shared_ptr<virtual_vertex_delegate>& orthogonal_state)
{
    auto s = std::dynamic_pointer_cast<state_delegate>(orthogonal_state);
    if (s) {
        return s->regions;
    }
    return std::vector<std::shared_ptr<virtual_region_delegate>>();
}

void pseudostate_delegate::validate() const
{
    virtual_vertex_delegate::validate();
    switch(type) {
    case pseudostate_kind::initial:
    case pseudostate_kind::entry_point:
    case pseudostate_kind::exit_point:
    {
        if (transitions.size() != 1) {
            throw_exception<not_allowed>("must have one and only one outgoing transition");
        }
        auto& t = transitions.front();
        auto target = t->target();
        auto region = owner<virtual_region_delegate>();
        if (!region) {
            throw_exception<internal_error>("has unknown owner type");
        }
        auto parent_state = region->owner<virtual_vertex_delegate>();
        if (parent_state) {
            if (parent_state == target) {
                throw_exception<not_allowed>("cannot have its parent state as target");
            }
            if (type != pseudostate_kind::initial && is_orthogonal_state(region)) {
                throw_exception<not_allowed>("cannot be a substate of an orthogonal state");
            }
        } else if (type != pseudostate_kind::initial) {
            throw_exception<not_allowed>("must be a sub state of a composite or sub machine state");
        }
        if (type == pseudostate_kind::exit_point) {
            auto parent_region = parent_state->owner<virtual_region_delegate>();
            if (parent_region) {
                if (!parent_region->contains(target)) {
                    throw_exception<not_allowed>("must have its target state among its parent's enclosing region");
                }
                auto target_region = target->owner< virtual_region_delegate>();
                if (region->contains(target) || target_region == region) {
                    throw_exception<not_allowed>("must have its target state outside its own enclosing region");
                }
            } else {
                parent_state->throw_exception<internal_error>("has unknown owner type");
            }
        } else {
            if (!region->contains(target)) {
                throw_exception<not_allowed>("must have its target state among its own enclosing region");
            }
        }
        break;
    }
    case pseudostate_kind::choice:
    case pseudostate_kind::junction:
    {
        if (transitions.empty()) {
            throw_exception<not_allowed>("must have outgoing transitions");
        }
        bool def(false);
        for(const auto& t : transitions)
        {
            if (!t->guarded) {
                if (def) {
                    throw_exception<not_allowed>("cannot have multiple default transitions");
                }
                def = true;
            }
        }
        if (!def) {
            throw_exception<not_allowed>("must have a default outgoing transition");
        }
        break;
    }
    case pseudostate_kind::fork:
    {
        if (transitions.size() < 2) {
            throw_exception<not_allowed>("must have at least two outgoing transitions");
        }
        auto target_state = transitions.front()->target();
        auto region = target_state->owner<virtual_region_delegate>();
        if (!is_orthogonal_state(region)) {
            throw_exception<not_allowed>("must have outgoing transitions to sub states of an orthogonal state");
        }
        auto orthogonal_state = region->owner<virtual_vertex_delegate>();
        auto regions = orthogonal_regions(orthogonal_state);
        std::unordered_map<std::shared_ptr<virtual_region_delegate>, bool> map;
        std::transform(regions.begin(), regions.end(), std::inserter(map, map.begin()), [](const std::shared_ptr<virtual_region_delegate>& r) {
            return std::make_pair(r, false);
        });
        for(const auto& t : transitions)
        {
            target_state = t->target();
            auto r = target_state->owner<virtual_region_delegate>();
            auto it = map.find(r);
            if (it != map.end()) {
                if (it->second) {
                    throw_exception<not_allowed>("must have all its outgoing transitions to different regions of an orthogonal state");
                }
                it->second = true;
            } else {
                throw_exception<not_allowed>("must have all its outgoing transitions to different regions of the same orthogonal state");
            }
        }
        for(const auto& p : map)
        {
            if (!p.second) {
                throw_exception<not_allowed>("must have all its outgoing transitions to different regions of an orthogonal state");
            }
        }
        break;
    }
    case pseudostate_kind::join:
    {
        if (transitions.size() != 1) {
            throw_exception<not_allowed>("must have one and only one outgoing transition");
        }
        break;
    }
    case pseudostate_kind::shallow_history:
    case pseudostate_kind::deep_history:
    {
        if (transitions.size() > 1) {
            throw_exception<not_allowed>("cannot have multiple default transitions");
        }
        if (!transitions.empty()) {
            auto& t = transitions.front();
            auto target = t->target();
            auto region = owner<virtual_region_delegate>();
            if (!region) {
                throw_exception<internal_error>("has unknown owner type");
            }
            if (!region->contains(target)) {
                throw_exception<not_allowed>("must have its target state among its own enclosing region");
            }
            auto parent_state = region->owner<virtual_vertex_delegate>();
            if (!parent_state) {
                throw_exception<not_allowed>("must be a sub state of a composite, orthogonal or sub machine state");
            }
        }
        break;
    }
    default:
        ;
    }
    for(const auto& t : transitions)
    {
        auto tgt = t->target();
        if (!tgt) {
            throw_exception<not_allowed>("cannot have internal transitions");
        } else if (tgt.get() == this) {
            throw_exception<not_allowed>("cannot transition to itself");
        }
    }
}

std::string pseudostate_delegate::type_name() const
{
    switch(type) {
    case pseudostate_kind::initial:
        return "initial pseudostate";
    case pseudostate_kind::terminate:
        return "terminate pseudostate";
    case pseudostate_kind::entry_point:
        return "entry point pseudostate";
    case pseudostate_kind::exit_point:
        return "exit point pseudostate";
    case pseudostate_kind::choice:
        return "choice pseudostate";
    case pseudostate_kind::join:
        return "join pseudostate";
    case pseudostate_kind::fork:
        return "fork pseudostate";
    case pseudostate_kind::junction:
        return "junction pseudostate";
    case pseudostate_kind::shallow_history:
        return "shallow history pseudostate";
    case pseudostate_kind::deep_history:
        return "deep history pseudostate";
    }
    return "";
}

pseudostate_delegate::pseudostate_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : virtual_vertex_delegate(std::move(n), o)
{
}

pseudostate_delegate::pseudostate_delegate(std::string n)
    : virtual_vertex_delegate(std::move(n))
{
}

std::shared_ptr<pseudostate_delegate> get_pseudostate(pseudostate_kind type, const std::vector<std::shared_ptr<virtual_vertex_delegate>>& states)
{
    for(const auto& state: states)
    {
        auto pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(state);
        if (pseudostate && pseudostate->type == type) {
            return pseudostate;
        }
    }
    return std::shared_ptr<pseudostate_delegate>();
}

}

pseudostate::pseudostate(std::shared_ptr<delegate_type> d)
    : detail::virtual_vertex<delegate_type>(std::move(d))
{
}

pseudostate::pseudostate(pseudostate_kind t, std::string n)
    : detail::virtual_vertex<delegate_type>(std::make_shared<delegate_type>(std::move(n)))
{
    delegate->type = t;
}

pseudostate_kind pseudostate::type() const
{
    return delegate->type;
}

pseudostate make_pseudostate(pseudostate_kind t, std::string name)
{
    return pseudostate(t, std::move(name));
}

pseudostate make_initial_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::initial, std::move(name));
}

pseudostate make_terminate_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::terminate, std::move(name));
}

pseudostate make_entry_point_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::entry_point, std::move(name));
}

pseudostate make_exit_point_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::exit_point, std::move(name));
}

pseudostate make_choice_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::choice, std::move(name));
}

pseudostate make_join_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::join, std::move(name));
}

pseudostate make_fork_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::fork, std::move(name));
}

pseudostate make_junction_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::junction, std::move(name));
}

pseudostate make_shallow_history_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::shallow_history, std::move(name));
}

pseudostate make_deep_history_pseudostate(std::string name)
{
    return make_pseudostate(pseudostate_kind::deep_history, std::move(name));
}

}
}
