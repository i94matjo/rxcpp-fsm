/*! \file  rx-fsm-region.cpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/


#include "rxcpp/fsm/rx-fsm-region.hpp"

namespace rxcpp {

namespace fsm {

namespace detail {

std::string region_delegate::type_name() const
{
    return "region";
}

region_delegate::region_delegate(std::string n, const std::shared_ptr<element_delegate>& o)
    : virtual_region_delegate(std::move(n), o)
{
}

region_delegate::region_delegate(std::string n)
    : virtual_region_delegate(std::move(n))
{
}

}

region::region(std::shared_ptr<delegate_type> d)
    : detail::virtual_region<delegate_type>(std::move(d))
{
}

region::region(std::string n)
    : detail::virtual_region<delegate_type>(std::make_shared<delegate_type>(std::move(n)))
{
}


region make_region(std::string name)
{
    return region(std::move(name));
}

}
}
