/*! \file  rx-fsm-predef.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-predef.hpp"
#include "rxcpp/fsm/rx-fsm-state_machine.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

std::shared_ptr<const state_machine_delegate> element_delegate::state_machine() const
{
   auto o = this->owner<element_delegate>();
   if (!o) {
       const auto* sm = dynamic_cast<const state_machine_delegate*>(this);
       if (sm) {
           return sm->shared_from_this();
       }
       return std::shared_ptr<const state_machine_delegate>();
   }
   auto p = o->owner<element_delegate>();
   while (p)
   {
       o = p;
       p = o->owner<element_delegate>();
   }
   return std::dynamic_pointer_cast<const state_machine_delegate>(o);
}

bool element_delegate::is_assembled() const
{
    auto sm = state_machine();
    if (sm) {
        return sm->assembled.load();
    }
    return false;
}

element_delegate::element_delegate(std::string n, const std::shared_ptr<this_type>& o)
    : name(std::move(n))
    , owner_(o)
{
}

element_delegate::element_delegate(std::string n)
    : name(std::move(n))
{
}

std::string element_delegate::exception_prefix() const
{
    std::ostringstream s;
    auto sm = state_machine();
    if (sm) {
        s << "In state machine '" << sm->name << "': ";
    }
    s << type_name() << " '" << name << "' ";
    return s.str();
}

internal_error::internal_error(const std::string& msg)
    : std::logic_error(msg)
{
}

}

not_allowed::not_allowed(const std::string& msg)
    : std::logic_error(msg)
{
}

join_error::join_error(const std::string& msg)
    : std::runtime_error(msg)
{
}

deleted_error::deleted_error(const std::string& msg)
    : std::runtime_error(msg)
{
}

state_error::state_error(const std::string& msg)
    : std::runtime_error(msg)
{
}

}
}
