/*! \file  rx-fsm-delegates.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_DELEGATES_HPP)
#define RX_FSM_DELEGATES_HPP

#include "rx-fsm-predef.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

struct transition_delegate;
struct virtual_vertex_delegate;

struct virtual_region_delegate : public element_delegate
{
    typedef virtual_region_delegate this_type;

    std::vector<std::shared_ptr<virtual_vertex_delegate>> sub_states;

    bool contains(const std::shared_ptr<virtual_vertex_delegate>& sub_state) const;

    explicit virtual_region_delegate(std::string n, const std::shared_ptr<element_delegate>& o);

    explicit virtual_region_delegate(std::string n);

    virtual_region_delegate() = default;

    virtual ~virtual_region_delegate() = default;
};

struct virtual_vertex_delegate : public element_delegate
{
    typedef virtual_vertex_delegate this_type;

    std::vector<std::shared_ptr<transition_delegate>> transitions;

    void add_transition(const std::shared_ptr<transition_delegate>& t);

    virtual void check_transition_target(const std::shared_ptr<transition_delegate>& t) const = 0;

    virtual void check_transition_source(const std::shared_ptr<transition_delegate>& t) const = 0;

    virtual void validate() const;

    explicit virtual_vertex_delegate(std::string n, const std::shared_ptr<element_delegate>& o);
    explicit virtual_vertex_delegate(std::string n);

    virtual_vertex_delegate() = default;

    virtual ~virtual_vertex_delegate() = default;
};

}
}
}

#endif
