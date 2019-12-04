/*! \file  rx-fsm-predef.hpp

    \copyright  Copyright (c) 2019, emJay Software Consulting AB. All rights reserved. Use of this source code is governed by the Apache-2.0 license, that can be found in the LICENSE.md file.
    \author     Mattias Johansson
*/

#pragma once

#if !defined(RX_FSM_PREDEF_HPP)
#define RX_FSM_PREDEF_HPP

#include "rxcpp/rx.hpp"
#include <sstream>

namespace rxcpp {

namespace fsm {

namespace detail {

struct state_machine_delegate;

struct element_delegate
{
    typedef element_delegate this_type;

    std::string name;
    std::weak_ptr<element_delegate> owner_;

    std::shared_ptr<const state_machine_delegate> state_machine() const;

    bool is_assembled() const;

    template<class Delegate>
    std::shared_ptr<Delegate> owner() const
    {
        return std::dynamic_pointer_cast<Delegate>(owner_.lock());
    }

    template<class Delegate>
    bool is_owned_by(const std::shared_ptr<Delegate>& owner) const
    {
        auto o = owner_.lock();
        while (o)
        {
            if (o == owner) {
                return true;
            }
            o = o->owner_.lock();
        }
        return false;
    }

    virtual std::string type_name() const = 0;

    template<class Exception>
    void throw_exception(const std::string& msg) const
    {
        std::ostringstream s;
        s << exception_prefix() << msg;
        throw Exception(s.str());
    }

    explicit element_delegate(std::string n, const std::shared_ptr<this_type>& o);

    explicit element_delegate(std::string n);

    element_delegate() = default;

    virtual ~element_delegate() = default;

private:

    std::string exception_prefix() const;
};

}

/*!  \brief  Base class of all fsm elements, such as states, transitions etc.

     \tparam Delegate  Delegate class for implementation specifics.
 */
template<class Delegate>
class element
{
protected:

    typedef Delegate delegate_type;
    typedef element<delegate_type> this_type;

    std::shared_ptr<delegate_type> delegate;

    element() = default;

    element(std::shared_ptr<Delegate> d)
        : delegate(std::move(d))
    {
    }

public:
    /*!  \brief  Returns the implementation specific delegate object.

         \return  The implementation specific delegate object.
     */
    const std::shared_ptr<delegate_type>& operator()() const
    {
       return delegate;
    }

    /*!  \brief  Returns name of the element.

         \return  The name of the element.
     */
    const std::string& name() const
    {
        return delegate->name;
    }

    /*!  \brief  Default destructor
     */
    virtual ~element() = default;

};

/*!  \brief  Exception thrown at pre assemble time when a not allowed construction is used.
 */
class not_allowed : public std::logic_error
{
public:
    /*!  \brief  Exception constructor.

         \param msg  The exception message.
     */
    explicit not_allowed(const std::string& msg);
};

/*!  \brief  Exception thrown when a join cannot be completed.
 */
class join_error : public std::runtime_error
{
public:
    /*!  \brief  Exception constructor.

         \param msg  The exception message.
     */
    explicit join_error(const std::string& msg);
};

/*!  \brief  Exception thrown when a transition element is used after state machine is deleted.
 */
class deleted_error : public std::runtime_error
{
public:
    /*!  \brief  Exception constructor.

         \param msg  The exception message.
     */
    explicit deleted_error(const std::string& msg);
};

/*!  \brief  Exception thrown when wrong state type is expected when trying to extract source and/or target states of a transition.
 */
class state_error : public std::runtime_error
{
public:
    /*!  \brief  Exception constructor.

         \param msg  The exception message.
     */
    explicit state_error(const std::string& msg);
};

namespace detail {

struct tag_state {};
struct state_base
{
    typedef tag_state state_tag;
};

struct tag_pseudostate {};
struct pseudostate_base
{
    typedef tag_pseudostate pseudostate_tag;
};

struct tag_region {};
struct region_base
{
    typedef tag_region region_tag;
};

struct tag_state_machine {};
struct state_machine_base
{
    typedef tag_state_machine state_machine_tag;
};

template<class Trigger>
struct empty_action
{
    typedef rxu::value_type_t<Trigger> value_type;
    void operator()(const value_type&) const {}
};

template<>
struct empty_action<void>
{
    void operator()() const {}
};

template<class Trigger>
struct empty_guard
{
    typedef rxu::value_type_t<Trigger> value_type;
    bool operator()(const value_type&) const { return true; }
};

template<>
struct empty_guard<void>
{
    bool operator()() const { return true; }
};

}

/*!  \brief  Trait for determining if type \a T is a state.

     \tparam T  Type to check.
 */
template<class T>
class is_state
{
    struct not_void {};
    template<class C>
    static typename C::state_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), detail::tag_state*>::value;
};

/*!  \brief  Trait for determining if type \a T is a pseudo state.

     \tparam T  Type to check.
 */
template<class T>
class is_pseudostate
{
    struct not_void {};
    template<class C>
    static typename C::pseudostate_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), detail::tag_pseudostate*>::value;
};

/*!  \brief  Trait for determining if type \a T is a vertex.

     \tparam T  Type to check.
 */
template<class T>
class is_vertex
{
public:
    static const bool value = is_state<T>::value || is_pseudostate<T>::value;
};

template<class... U>
class is_vertices;

template<class T>
class is_vertices<T>
{
public:
    static const bool value = is_vertex<T>::value;
};

/*!  \brief  Trait for determining if all types are vertices.

     \tparam T  First type to check.
     \tparam U  Rest of types to check
 */
template<class T, class... U>
class is_vertices<T, U...>
{
public:
    static const bool value = is_vertex<T>::value && is_vertices<U...>::value;
};

/*!  \brief  Trait for determining if type \a T is a region.

     \tparam T  Type to check.
 */
template<class T>
class is_region
{
    struct not_void {};
    template<class C>
    static typename C::region_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), detail::tag_region*>::value;
};

template<class... U>
class is_regions;

template<class T>
class is_regions<T>
{
public:
    static const bool value = is_region<T>::value;
};

/*!  \brief  Trait for determining if all types are regions.

     \tparam T  First type to check.
     \tparam U  Rest of types to check
 */
template<class T, class... U>
class is_regions<T, U...>
{
public:
    static const bool value = is_region<T>::value && is_regions<U...>::value;
};

/*!  \brief  Trait for determining if type \a T is a state machine.

     \tparam T  Type to check.
 */
template<class T>
class is_state_machine
{
    struct not_void {};
    template<class C>
    static typename C::state_machine_tag* check(int);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<rxu::decay_t<T>>(0)), detail::tag_state_machine*>::value;
};

/*!  \brief  Trait for determining if \a F is a callable returning void taking an argument of type \a T.

     \tparam T  Type of the argument.
     \tparam F  Callable.
 */
template<class T, class F>
class is_action_of
{
    struct not_void {};
    template<class CT, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CT*)nullptr));
    template<class CT, class CF>
    static not_void check(...);
public:
    typedef decltype(check<T, rxu::decay_t<F>>(0)) detail_result;
    static const bool value = std::is_same<detail_result, void>::value;
};

/*!  \brief  Trait for determining if \a F is a callable returning void taking no arguments.

     \tparam F  Callable.
 */
template<class F>
class is_action_of<void, F>
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)());
    template<class CF>
    static not_void check(...);
public:
    typedef decltype(check<rxu::decay_t<F>>(0)) detail_result;
    static const bool value = std::is_same<detail_result, void>::value;
};

/*!  \brief  Trait for determining if \a F is a callable returning bool taking an argument of type \a T.

     \tparam T  Type of the argument.
     \tparam F  Callable.
 */
template<class T, class F>
class is_guard_of
{
    struct not_void {};
    template<class CT, class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)(*(CT*)nullptr));
    template<class CT, class CF>
    static not_void check(...);
public:
    typedef decltype(check<T, rxu::decay_t<F>>(0)) detail_result;
    static const bool value = std::is_same<detail_result, bool>::value;
};

/*!  \brief  Trait for determining if \a F is a callable returning bool taking no arguments.

     \tparam F  Callable.
 */
template<class F>
class is_guard_of<void, F>
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)());
    template<class CF>
    static not_void check(...);
public:
    typedef decltype(check<rxu::decay_t<F>>(0)) detail_result;
    static const bool value = std::is_same<detail_result, bool>::value;
};

/*!  \brief  Trait for determining if \a F is a callable returning void taking no arguments.

     \tparam F  Callable.
 */
template<class F>
class is_on_entry_or_exit
{
    struct not_void {};
    template<class CF>
    static auto check(int) -> decltype((*(CF*)nullptr)());
    template<class CF>
    static not_void check(...);
public:
    static const bool value = std::is_same<decltype(check<rxu::decay_t<F>>(0)), void>::value;
};

/*!  \brief  Trait for determining if type \a T is equality comparable.

     \tparam T  Type to check.
 */
template<class T>
class is_equality_comparable
{
    struct not_void {};
    template<class C>
    static auto check(int) -> decltype(std::declval<C&>() == std::declval<C&>(), (void)0);
    template<class C>
    static not_void check(...);
public:
    static const bool value = std::is_same<decltype(check<rxu::decay_t<T>>(0)), void>::value;
};

namespace detail {

class internal_error : public std::logic_error
{
public:
    explicit internal_error(const std::string& msg);
};

}
}
}

#define RX_FSM_EQ_OPERATOR(elem) \
    inline bool operator==(const elem &lhs, const elem &rhs) \
    { \
        return lhs.delegate == rhs.delegate; \
    }

#define RX_FSM_NE_OPERATOR(elem) \
    inline bool operator!=(const elem &lhs, const elem &rhs) \
    { \
        return !(lhs == rhs); \
    }

#define RX_FSM_LT_OPERATOR(elem) \
    inline bool operator<(const elem &lhs, const elem &rhs) \
    { \
        return lhs.delegate < rhs.delegate; \
    }

#define RX_FSM_FRIEND_OPERATORS(elem) \
    friend bool operator==(const elem &, const elem&); \
    friend bool operator!=(const elem &, const elem&); \
    friend bool operator<(const elem &, const elem&); \
    friend struct std::hash< elem >;


#define RX_FSM_OPERATORS(elem) \
    RX_FSM_EQ_OPERATOR(elem) \
    RX_FSM_NE_OPERATOR(elem) \
    RX_FSM_LT_OPERATOR(elem)

#define RX_FSM_HASH(elem) \
    namespace std \
    { \
    template <> \
    struct hash< elem > \
    { \
        size_t operator()(const elem &e) const noexcept \
        { \
            return std::hash<decltype(e.delegate)>()(e.delegate); \
        } \
    }; \
    }

#endif
