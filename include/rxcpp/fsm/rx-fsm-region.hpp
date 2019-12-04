/*! \file  rx-fsm-region.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_REGION_HPP)
#define RX_FSM_REGION_HPP

#include "rx-fsm-delegates.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

struct region_delegate : public virtual_region_delegate
{
    virtual std::string type_name() const override;

    explicit region_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit region_delegate(std::string n);
};

template<class Delegate>
class virtual_region : public element<Delegate>
{
public:

    typedef virtual_region<Delegate> this_type;

private:

    bool check_sub_state(const std::shared_ptr<detail::virtual_vertex_delegate>& sub_state)
    {
        const auto& sub_states = this->delegate->sub_states;
        auto it = std::find_if(sub_states.begin(), sub_states.end(), [&sub_state](const std::shared_ptr<detail::virtual_vertex_delegate>& s) {
            return s == sub_state || s->name == sub_state->name;
        });
        return it == sub_states.end();
    }

    template<class... SubStateN>
    struct WithSubState;

    template<class SubState0>
    struct WithSubState<SubState0>
    {
        this_type& operator()(this_type* self, SubState0 sub_state0)
        {
            auto s = sub_state0();
            if (self->check_sub_state(s)) {
                self->delegate->sub_states.push_back(s);
                s->owner_ = self->delegate;
            } else {
                std::ostringstream msg;
                msg << "has a sub state '" << s->name << "' that is equal to or has the same name as a sibling state";
                self->delegate->template throw_exception<not_allowed>(msg.str());
            }
            return *self;
        }
    };

    template<class SubState0, class... SubStateN>
    struct WithSubState<SubState0, SubStateN...>
    {
        this_type& operator()(this_type* self, SubState0 sub_state0, SubStateN... sub_stateN)
        {
            WithSubState<SubState0>()(self, std::move(sub_state0));
            WithSubState<SubStateN...>()(self, std::move(sub_stateN)...);
            return *self;
        }
    };

protected:

    virtual_region() = default;

    virtual_region(std::shared_ptr<Delegate> d)
        : element<Delegate>(std::move(d))
    {
    }

    // sub states

    template<class SubState0, class... SubStateN>
    auto with_sub_state(SubState0 sub_state0, SubStateN... sub_stateN)
        -> typename std::enable_if<is_vertices<SubState0, SubStateN...>::value, this_type&>::type
    {
        if (this->delegate->is_assembled()) {
            this->delegate->template throw_exception<not_allowed>("state machine already assembled");
        }
        return WithSubState<SubState0, SubStateN...>()(this, std::move(sub_state0), std::move(sub_stateN)...);
    }

};

}

/*!  \brief  A region denotes a behavior fragment that may execute concurrently with its orthogonal regions.

     Two or more regions are orthogonal to each other if they are either owned by the same state or, at the topmost level, by the same
     state machine. A region becomes active (i.e., it begins executing) either when its owning state is entered or, if it is
     directly owned by a state machine (i.e., it is a top level region), when its owning state machine starts executing. Each
     region owns a set of vertices and transitions, which determine the behavioral flow within that region. It may have its
     own initial pseudostate as well as its own final state.

     A default activation of a region occurs if the region is entered implicitly, that is, it is not entered through an incoming
     transition that terminates on one of its component vertices (e.g., a state or a history pseudostate), but either
       - through a (local or external) transition that terminates on the containing state or,
       - in case of a top level region, when the state machine starts executing.

     Default activation means that execution starts with the transition originating from the initial pseudostate of the region,
     if one is defined. However, no specific approach is defined if there is no initial pseudostate that exists within the
     region. One possible approach is to deem the model ill defined. An alternative is that the region remains inactive,
     although the state that contains it is active. In other words, the containing composite state is treated as a simple (leaf)
     state.

     Conversely, an explicit activation occurs when a region is entered by a transition terminating on one of the region’s
     contained vertices. When one region of an orthogonal state is activated explicitly, this will result in the default
     activation of all of its orthogonal regions, unless those regions are also entered explicitly (multiple orthogonal regions
     can be entered explicitly in parallel through transitions originating from the same fork pseudostate).

     \see <a href='https://www.omg.org/spec/UML/2.5.1/PDF'>OMG UML Specification</a>

     \note  The class uses reference semantics and is eqaulity comparable, and can be used as keys in maps, sets including unordered.
 */
class region final : public detail::virtual_region<detail::region_delegate>
                   , public detail::region_base
{
private:

    region() = delete;

    explicit region(std::shared_ptr<delegate_type> d);

    explicit region(std::string n);

    friend class state;
    friend class state_machine;
    friend region make_region(std::string);
    RX_FSM_FRIEND_OPERATORS(region)

public:

    typedef region this_type;

    /*!  \brief  Adds sub states to this region.

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
       return static_cast<this_type&>(detail::virtual_region<delegate_type>::with_sub_state(sub_state0, sub_stateN...));
    }

};

/*!  \brief Creates a region.

     \param name  The name of the region.

     \return  A \a region instance.
 */
region make_region(std::string name);

RX_FSM_OPERATORS(region)

}
}

RX_FSM_HASH(rxcpp::fsm::region)
#endif
