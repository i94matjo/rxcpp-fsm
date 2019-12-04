/*! \file  rx-fsm-state_machine.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-state_machine.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

void state_machine_delegate::get_join_pseudostates(const std::shared_ptr<virtual_vertex_delegate>& state)
{
    auto pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(state);
    if (pseudostate && pseudostate->type == pseudostate_kind::join && join_pseudostates.find(pseudostate) == join_pseudostates.end()) {
        join_pseudostates[pseudostate] = join_pseudostate_map::mapped_type();
    }
    for(const auto& t : state->transitions)
    {
        pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(t->target());
        if (pseudostate && pseudostate->type == pseudostate_kind::join) {
            join_pseudostates[pseudostate].push_back(state);
        }
    }
}

std::shared_ptr<state_machine_delegate::current_state> state_machine_delegate::find_common_ancestor(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& target)
{
    const auto& ancestors = state_ancestors[target];
    auto cur = current;
    while (cur) {
        auto parent = cur->parent.lock();
        if (parent) {
            if (parent->state) {
                auto it = std::find(ancestors.begin(), ancestors.end(), parent->state);
                if (it != ancestors.end()) {
                    return cur;
                }
            } else {
                break;
            }
        }
        cur = parent;
    }
    return this->current;
}

void state_machine_delegate::determine_exit_order_recursively(const std::shared_ptr<current_state>& current, std::multimap<int, const std::shared_ptr<current_state>>& map, int level)
{
    if (current->state) {
        map.insert(std::make_pair(level--, current));
        for(const auto& child : current->children)
        {
            determine_exit_order_recursively(child, map, level);
        }
    }
}

std::multimap<int, const std::shared_ptr<state_machine_delegate::current_state>> state_machine_delegate::determine_exit_order(const std::shared_ptr<current_state>& current)
{
    std::multimap<int, const std::shared_ptr<current_state>> map;
    determine_exit_order_recursively(current, map, 0);
    return map;
}

void state_machine_delegate::get_deep_history_recursively(const std::shared_ptr<current_state>& current, std::vector<std::shared_ptr<virtual_vertex_delegate>>& history)
{
    if (current->state) {
        if (current->children.empty()) {
            history.push_back(current->state);
        } else {
            for(const auto& child : current->children)
            {
                get_deep_history_recursively(child, history);
            }
        }
    }
}

std::vector<std::shared_ptr<virtual_vertex_delegate>> state_machine_delegate::get_deep_history(const std::shared_ptr<current_state>& current)
{
    std::vector<std::shared_ptr<virtual_vertex_delegate>> history;
    get_deep_history_recursively(current, history);
    return history;
}

void state_machine_delegate::exit_region(const std::shared_ptr<current_state>& current)
{
    auto parent = current->parent.lock();
    if (parent) {
        for(const auto& child : parent->children)
        {
            auto cs = child->lifetime;
            parent->state_lifetime.remove(cs.get_weak());
            cs.unsubscribe();
        }
        parent->children.clear();
    } else {
        current->lifetime.unsubscribe();
    }
}

void state_machine_delegate::exit_state(const std::shared_ptr<current_state>& common, const std::shared_ptr<current_state>& current)
{
    auto s = std::dynamic_pointer_cast<state_delegate>(current->state);
    if (s) {
       if (!current->entered) {
           s->on_entry();
       }
       s->on_exit();
       current->entered = false;
    }
    auto cs = current->state_lifetime;
    current->lifetime.remove(cs.get_weak());
    cs.unsubscribe();
    current->state.reset();
    if (current != common) {
        exit_region(current);
    }
}

void state_machine_delegate::exit_states_recursively(const std::shared_ptr<current_state>& current)
{
    auto order = determine_exit_order(current);
    for(const auto& o : order)
    {
        auto r = o.second->region.lock();
        if (r) {
            auto deep = get_pseudostate(pseudostate_kind::deep_history, r->sub_states);
            if (deep) {
                deep_history_pseudostates[deep] = get_deep_history(o.second);
            }
            auto shallow = get_pseudostate(pseudostate_kind::shallow_history, r->sub_states);
            if (shallow) {
                shallow_history_pseudostates[shallow] = o.second->state;
            }
        }
    }
    for (const auto& o : order)
    {
        exit_state(current, o.second);
    }
}

std::vector<std::shared_ptr<virtual_vertex_delegate>> state_machine_delegate::determine_target_states(const std::shared_ptr<virtual_vertex_delegate>& target)
{
    std::vector<std::shared_ptr<virtual_vertex_delegate>> targets;
    auto pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(target);
    if (pseudostate) {
        switch (pseudostate->type) {
        case pseudostate_kind::shallow_history:
        {
            const auto& history_state = shallow_history_pseudostates[pseudostate];
            if (history_state) {
                targets.push_back(history_state);
            } else {
                if (pseudostate->transitions.empty()) {
                    auto region = target->owner<virtual_region_delegate>();
                    if (region) {
                        auto initial = get_pseudostate(pseudostate_kind::initial, region->sub_states);
                        if (initial) {
                            targets.push_back(initial);
                        }
                    }
                } else {
                    const auto& t = pseudostate->transitions.front();
                    targets.push_back(t->target());
                    auto subscriber = subject.get_subscriber();
                    if (subscriber.is_subscribed()) {
                        subscriber.on_next(transition(t));
                    }
                    t->execute_action();
                }
            }
            break;
        }
        case pseudostate_kind::deep_history:
        {
            const auto& history_states = deep_history_pseudostates[pseudostate];
            if (!history_states.empty()) {
                return history_states;
            }
            if (pseudostate->transitions.empty()) {
                auto region = target->owner<virtual_region_delegate>();
                if (region) {
                    auto initial = get_pseudostate(pseudostate_kind::initial, region->sub_states);
                    if (initial) {
                        targets.push_back(initial);
                    }
                }
            } else {
                const auto& t = pseudostate->transitions.front();
                targets.push_back(t->target());
                auto subscriber = subject.get_subscriber();
                if (subscriber.is_subscribed()) {
                    subscriber.on_next(transition(t));
                }
                t->execute_action();
            }
            break;
        }
        case pseudostate_kind::fork:
        {
            for(const auto& t : pseudostate->transitions)
            {
                auto targets_ = determine_target_states(t->target());
                targets.insert(targets.end(), targets_.begin(), targets_.end());
                auto subscriber = subject.get_subscriber();
                if (subscriber.is_subscribed()) {
                    subscriber.on_next(transition(t));
                }
                t->execute_action();
            }
            break;
        }
        default:
            targets.push_back(target);
            break;
        }
    } else {
        targets.push_back(target);
    }
    for(const auto& tgt : targets)
    {
        auto s = std::dynamic_pointer_cast<state_delegate>(tgt);
        if (s && s->type != state_delegate::simple) {
            targets.erase(std::remove(targets.begin(), targets.end(), tgt));
            for(const auto& r : s->regions)
            {
                auto initial = get_pseudostate(pseudostate_kind::initial, r->sub_states);
                if (initial) {
                    targets.push_back(initial);
                }
            }
        }
    }
    return targets;
}

void state_machine_delegate::state_transition(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& target, const std::shared_ptr<transition_delegate::action>& action)
{
    // make transition
    // if transition is to a terminate pseudostate, preform action and terminate directly
    auto pseudostate = std::dynamic_pointer_cast<pseudostate_delegate>(target);
    if (pseudostate && pseudostate->type == pseudostate_kind::terminate) {
        action->execute();
        auto subscriber = subject.get_subscriber();
        if (subscriber.is_subscribed()) {
            subscriber.on_completed();
        }
        this->current->lifetime.unsubscribe();
        return;
    }
    // if transition to a join pseudostate all orthogonal regions must have been exited in order to perform the actual transition
    // if transition to a final state, do not exit the parent state unless all orthogonal regions are exited
    auto common = find_common_ancestor(current, target);
    auto final = std::dynamic_pointer_cast<final_state_delegate>(target);
    std::shared_ptr<current_state> final_parent;
    bool all_regions_complete(true);
    if (final || (pseudostate && pseudostate->type == pseudostate_kind::join)) {
        auto r = final ? target->owner<virtual_region_delegate>() : current->state->owner<virtual_region_delegate>();
        auto current_region = r ? find_current_region(common, r) : std::shared_ptr<current_state>();
        if (current_region) {
            current_region->status = final ? await_finalize : await_join;
            final_parent = current_region->parent.lock();
            if (final_parent) {
                for(const auto& child : final_parent->children)
                {
                    auto status = child->status;
                    if (status == active) {
                        all_regions_complete = false;
                        break;
                    }
                    if (final && status == await_join) {
                        r->throw_exception<join_error>("cannot be finalized since another orthogonal region is awaiting join");
                    }
                    else if (!final && status == await_finalize) {
                        r->throw_exception<join_error>("cannot be joined since another orthogonal region is already finalized");
                    }
                }
            }
            if (!all_regions_complete) {
                common = current_region;
            }
        }
    }
    // exit to common
    exit_states_recursively(common);
    // perform action
    action->execute();
    // no transition if not all regions is complete
    if (!all_regions_complete) {
        return;
    }
    // if target is a final state unblock all completion transitions of parent state if all orthogonal regions is finalized
    if (final && final_parent && all_regions_complete) {
        auto completion_transitions = type_filtered_transitions(transition_delegate::completion, final_parent->state->transitions);
        auto cs = final_parent->state_lifetime;
        for(const auto& t : completion_transitions)
        {
            t->unblock();
            if (!cs.is_subscribed()) {
                return;
            }
        }
    }
    // determine "actual" target state(s)
    auto target_states = determine_target_states(target);
    // enter the target(s)
    enter_states_recursively(common, target_states);
}

std::shared_ptr<state_machine_delegate::current_state> state_machine_delegate::find_current_state(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_vertex_delegate>& source_state) const
{
    if (current->state == source_state) {
        return current;
    }
    for(const auto& child : current->children)
    {
        auto result = find_current_state(child, source_state);
        if (result) {
            return result;
        }
    }
    return std::shared_ptr<current_state>();
}

std::shared_ptr<state_machine_delegate::current_state> state_machine_delegate::find_current_region(const std::shared_ptr<current_state>& current, const std::shared_ptr<virtual_region_delegate>& source_region) const
{
    if (current->region.lock() == source_region) {
        return current;
    }
    for(const auto& child : current->children)
    {
        auto result = find_current_region(child, source_region);
        if (result) {
            return result;
        }
    }
    return std::shared_ptr<current_state>();
}

void state_machine_delegate::enter_state(const std::shared_ptr<current_state>& current)
{
    auto s = std::dynamic_pointer_cast<state_delegate>(current->state);
    if (s) {
        if (s->type != state_delegate::simple) {
            // block all completion transitions until all regions are completed
            auto completion_transitions = type_filtered_transitions(transition_delegate::completion, s->transitions);
            for(const auto& t : completion_transitions)
            {
                t->try_block();
            }
        }
    }
    std::weak_ptr<this_type> weak = shared_from_this();
    auto& observable = state_observable[current->state];
    auto on_next = [weak, current](const transition_delegate::transition_data& data) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        if (!current->entered) {
            auto s = std::dynamic_pointer_cast<state_delegate>(current->state);
            if (s) {
                current->entered = true;
                s->on_entry();
            }
        }
        const auto& source = std::get<0>(data);
        const auto& t = std::get<1>(data);
        const auto& action = std::get<2>(data);
        const auto& target = t->target();
        auto subscriber = self->subject.get_subscriber();
        if (subscriber.is_subscribed()) {
            subscriber.on_next(transition(t));
        }
        if (target) {
            auto s = self->find_current_state(current, source);
            self->state_transition(s, target, action);
        } else {
            action->execute();
        }
    };
    auto subscr = subject.get_subscriber();
    auto on_error = [subscr](std::exception_ptr e) {
        if (subscr.is_subscribed()) {
            subscr.on_error(std::move(e));
        }
    };
    current->state_lifetime = composite_subscription();
    observable.subscribe(current->state_lifetime, on_next, on_error);
    if (s && !current->entered) {
        current->entered = true;
        s->on_entry();
    }
}

void state_machine_delegate::enter_states_recursively(const std::shared_ptr<current_state>& current, const std::vector<std::shared_ptr<virtual_vertex_delegate>>& target_states)
{
    std::multimap<int, std::shared_ptr<current_state>> order;
    auto parent = current->parent.lock();
    for(const auto& target : target_states)
    {
        const auto& ancestors = state_ancestors[target];
        state_ancestor_map::mapped_type::const_iterator it;
        if (parent) {
            it = std::find(ancestors.begin(), ancestors.end(), parent->state);
            if (it == ancestors.end()) {
                throw_exception<internal_error>("illegal parent");
            }
            ++it;
        } else {
            it = ancestors.begin();
        }
        int level(0);
        auto cur = current;
        for(;;)
        {
            const auto& s = it != ancestors.end() ? *it : target;
            if (level > 0)
            {
                auto it_ = std::find_if(cur->children.begin(), cur->children.end(), [&s](const std::shared_ptr<current_state>& c) {
                    return c->state == s;
                });
                if (it_ == cur->children.end()) {
                    auto new_current = std::make_shared<current_state>();
                    new_current->status = active;
                    new_current->region = s->owner<virtual_region_delegate>();
                    new_current->parent = cur;
                    new_current->state = s;
                    new_current->entered = false;
                    cur->children.push_back(new_current);
                    order.insert(std::make_pair(level, new_current));
                    cur = new_current;
                } else {
                    cur = *it_;
                }
            } else if (cur->state != s) {
                if (cur->state) {
                    auto cs = cur->state_lifetime;
                    cur->lifetime.remove(cs.get_weak());
                    cur->state.reset();
                    cs.unsubscribe();
                }
                cur->state = s;
                order.insert(std::make_pair(level, cur));
            }
            if (it == ancestors.end()) {
                break;
            }
            ++level;
            ++it;
        }
    }
    for(const auto& o : order)
    {
        auto parent = o.second->parent.lock();
        enter_state(o.second);
        if (o.second->state_lifetime.is_subscribed()) {
            o.second->lifetime.add(o.second->state_lifetime);
            if (parent && parent->state) {
                parent->state_lifetime.add(o.second->state_lifetime);
            }
        } else {
            break;
        }
    }
}

void state_machine_delegate::guard_executed(const std::shared_ptr<virtual_vertex_delegate>& state) const
{
    if (state) {
        auto current = find_current_state(this->current, state);
        if (current && !current->entered)
        {
            auto s = std::dynamic_pointer_cast<state_delegate>(state);
            if (s) {
                current->entered = true;
                s->on_entry();
            }
        }
    }
}

void state_machine_delegate::validate()
{
    for(const auto& state : sub_states)
    {
        state->validate();
    }
    for(const auto& pair : join_pseudostates)
    {
        const auto& pseudostate = pair.first;
        const auto& states = pair.second;
        if (states.size() < 2) {
            pseudostate->throw_exception<not_allowed>("must have at least two incoming transitions");
        }
        auto region = states.front()->owner<region_delegate>();
        if (!region) {
            pseudostate->throw_exception<not_allowed>("must have incoming transitions from sub states of an orthogonal state");
        }
        std::unordered_map<std::shared_ptr<virtual_region_delegate>, bool> map;
        std::transform(states.begin(), states.end(), std::inserter(map, map.begin()), [](const std::shared_ptr<virtual_vertex_delegate>& s) {
            auto r = s->owner<virtual_region_delegate>();
            return std::make_pair(r, false);
        });
        for(const auto& state : states)
        {
            auto r = state->owner<virtual_region_delegate>();
            auto it = map.find(r);
            if (it != map.end()) {
                if (it->second) {
                    pseudostate->throw_exception<not_allowed>("must have all its incoming transitions from different regions of an orthogonal state");
                }
                it->second = true;
            } else {
                pseudostate->throw_exception<not_allowed>("must have all its incoming transitions from different regions of the same orthogonal state");
            }
        }
        for(const auto& p : map)
        {
            if (!p.second) {
                pseudostate->throw_exception<not_allowed>("must have all its incoming transitions from different regions of an orthogonal state");
            }
        }
    }
}

void state_machine_delegate::find_reachable_states_recursively(std::unordered_map<std::shared_ptr<virtual_vertex_delegate>, bool>& states_reached, const std::shared_ptr<virtual_vertex_delegate>& state) const
{
    for(const auto& t : state->transitions)
    {
        auto tgt = t->target();
        if (tgt) {
            auto it = states_reached.find(tgt);
            if (it == states_reached.end() || !it->second) {
                states_reached[tgt] = true;
                auto s = std::dynamic_pointer_cast<state_delegate>(tgt);
                if (s && s->type != state_delegate::simple) {
                    for(const auto& r : s->regions) {
                        auto initial = get_pseudostate(pseudostate_kind::initial, r->sub_states);
                        states_reached[initial] = false;
                        find_reachable_states_recursively(states_reached, initial);
                    }
                }
                find_reachable_states_recursively(states_reached, tgt);
                for(const auto& ancestor : state_ancestors.at(tgt))
                {
                    states_reached[ancestor] = false;
                    find_reachable_states_recursively(states_reached, ancestor);
                }
            }
        }
    }
}

std::vector<std::string> state_machine_delegate::find_unreachable_states() const
{
    auto initial = get_pseudostate(pseudostate_kind::initial, sub_states);
    std::unordered_map<std::shared_ptr<virtual_vertex_delegate>, bool> states_reached;
    states_reached[initial] = false;
    find_reachable_states_recursively(states_reached, initial);
    std::vector<std::string> state_names;
    for(const auto& pair : state_ancestors)
    {
        const auto& state = pair.first;
        auto it = states_reached.find(state);
        if (it == states_reached.end()) {
            state_names.push_back(state->name);
        }
    }
    return state_names;
}

std::string state_machine_delegate::type_name() const
{
    return "state machine";
}

state_machine_delegate::state_machine_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : virtual_region_delegate(std::move(n), o)
    , assembled(false)
    , subject(subject_lifetime)
{
}

state_machine_delegate::state_machine_delegate(std::string n)
    : virtual_region_delegate(std::move(n))
    , assembled(false)
    , subject(subject_lifetime)
{
}

state_machine_delegate::~state_machine_delegate()
{
    if (current) {
        current->lifetime.unsubscribe();
    } else {
        subject_lifetime.unsubscribe();
    }
}

}

state_machine::state_machine(std::shared_ptr<delegate_type> d)
    : detail::virtual_region<delegate_type>(std::move(d))
{
}

state_machine::state_machine(std::string n)
    : detail::virtual_region<delegate_type>(std::make_shared<delegate_type>(std::move(n)))
{
}

bool state_machine::is_assembled() const
{
    return delegate->assembled.load();
}

bool state_machine::is_terminated() const
{
    if (is_assembled() && delegate->current) {
        return !delegate->current->lifetime.is_subscribed();
    }
    return false;
}

void state_machine::terminate()
{
    if (is_assembled() && delegate->current) {
        auto subscriber = delegate->subject.get_subscriber();
        if (subscriber.is_subscribed()) {
            subscriber.on_completed();
        }
        delegate->current->lifetime.unsubscribe();
    }
}

std::vector<std::string> state_machine::find_unreachable_states() const
{
    if (!is_assembled()) {
        delegate->throw_exception<not_allowed>("must be assembled");
    }
    return delegate->find_unreachable_states();
}

state_machine make_state_machine(std::string name)
{
    return state_machine(std::move(name));
}

}
}
