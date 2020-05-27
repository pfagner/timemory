//  MIT License
//
//  Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.

/** \file mpl/apply.hpp
 * \headerfile mpl/apply.hpp "timemory/mpl/apply.hpp"
 * Provides the template meta-programming expansions heavily utilized by TiMemory
 *
 */

#pragma once

#include <functional>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

#include "timemory/mpl/function_traits.hpp"
#include "timemory/mpl/math.hpp"
#include "timemory/mpl/stl.hpp"

#if defined(__NVCC__)
#    define TIMEMORY_LAMBDA __host__ __device__
#    define TIMEMORY_HOST_LAMBDA __host__
#    define TIMEMORY_DEVICE_LAMBDA __device__
#else
#    define TIMEMORY_LAMBDA
#    define TIMEMORY_HOST_LAMBDA
#    define TIMEMORY_DEVICE_LAMBDA
#endif

//======================================================================================//

namespace tim
{
// clang-format off
namespace device { struct cpu; struct gpu; }  // namespace device
// clang-format on

namespace internal
{
//======================================================================================//

template <typename Ret>
struct apply
{
    //----------------------------------------------------------------------------------//
    //  invoke a function with a tuple
    //
    template <typename Fn, typename Tuple, size_t... Idx>
    static Ret invoke(Fn&& __f, Tuple&& __t, index_sequence<Idx...>)
    {
        return __f(std::get<Idx>(std::forward<Tuple>(__t))...);
    }

    //----------------------------------------------------------------------------------//
    // prefix with _sep
    //
    template <typename SepT, typename Arg,
              enable_if_t<std::is_same<Ret, std::string>::value, char> = 0>
    static Ret join_tail(std::stringstream& _ss, const SepT& _sep, Arg&& _arg)
    {
        _ss << _sep << std::forward<Arg>(_arg);
        return _ss.str();
    }

    //----------------------------------------------------------------------------------//
    // prefix with _sep
    //
    template <typename SepT, typename Arg, typename... Args,
              enable_if_t<std::is_same<Ret, std::string>::value, char> = 0>
    static Ret join_tail(std::stringstream& _ss, const SepT& _sep, Arg&& _arg,
                         Args&&... __args)
    {
        _ss << _sep << std::forward<Arg>(_arg);
        return join_tail<SepT, Args...>(_ss, _sep, std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    // don't prefix
    //
    template <typename SepT, typename Arg,
              enable_if_t<std::is_same<Ret, std::string>::value, char> = 0>
    static Ret join(std::stringstream& _ss, const SepT&, Arg&& _arg)
    {
        _ss << std::forward<Arg>(_arg);
        return _ss.str();
    }

    //----------------------------------------------------------------------------------//
    // don't prefix
    //
    template <typename SepT, typename Arg, typename... Args,
              enable_if_t<std::is_same<Ret, std::string>::value, char> = 0,
              enable_if_t<(sizeof...(Args) > 0), int>                  = 0>
    static Ret join(std::stringstream& _ss, const SepT& _sep, Arg&& _arg,
                    Args&&... __args)
    {
        _ss << std::forward<Arg>(_arg);
        return join_tail<SepT, Args...>(_ss, _sep, std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
};

//======================================================================================//

template <>
struct apply<void>
{
    using Ret = void;

    //----------------------------------------------------------------------------------//

    template <typename Tp, typename Tail>
    struct get_index_of;

    template <typename Tp, typename... Tail>
    struct get_index_of<Tp, std::tuple<Tp, Tail...>>
    {
        static constexpr int value = 0;
    };

    template <typename Tp, typename... Tail>
    struct get_index_of<Tp, std::tuple<Tp*, Tail...>>
    {
        static constexpr int value = 0;
    };

    template <typename Tp, typename Head, typename... Tail>
    struct get_index_of<Tp, std::tuple<Head, Tail...>>
    {
        static constexpr int value = 1 + get_index_of<Tp, std::tuple<Tail...>>::value;
    };

    template <typename Tp, typename... Tail>
    struct get_index_of<Tp, std::tuple<Tail...>>
    {
        static_assert(sizeof...(Tail) != 0, "Error! Type not found!");
    };

    //----------------------------------------------------------------------------------//

    template <typename Tp, typename Up, size_t N>
    struct out_of_order_T;

    template <template <typename, typename...> class Operator, typename TypeU,
              typename Type, typename... Types>
    struct out_of_order_T<Operator<Type, Types...>, TypeU, 1>
    {
        template <typename Tuple, typename... Args>
        static void access(Tuple&& __t, Args&&... __args)
        {
            using TupleT = decay_t<Tuple>;
            static_assert(std::tuple_size<TupleT>::value != 0, "Error! tuple_size = 0");
            constexpr int N = get_index_of<Type, TupleT>::value;
            using Tp        = decltype(std::get<N>(__t));
            Operator<Type, Types...>(std::forward<Tp>(std::get<N>(__t)),
                                     std::forward<Args>(__args)...);
        }
    };

    template <template <typename, typename...> class Operator, typename TypeU,
              typename Type, typename... Types>
    struct out_of_order_T<Operator<Type, Types...>, TypeU, 2>
    {
        template <typename TupleA, typename TupleB, typename... Args>
        static void access(TupleA&& __a, TupleB&& __b, Args&&... __args)
        {
            using TypeA = decay_t<TupleA>;
            using TypeB = decay_t<TupleB>;

            static_assert(std::tuple_size<TypeA>::value != 0, "Error! tuple_size = 0");
            static_assert(std::tuple_size<TypeB>::value != 0, "Error! tuple_size = 0");

            constexpr auto Na = get_index_of<Type, TypeA>::value;
            constexpr auto Nb = get_index_of<Type, TypeU>::value;
            using Ap          = decltype(std::get<Na>(__a));
            using Bp          = decltype(std::get<Nb>(__b));
            Operator<Type, Types...>(std::forward<Ap>(std::get<Na>(__a)),
                                     std::forward<Bp>(std::get<Nb>(__b)),
                                     std::forward<Args>(__args)...);
        }
    };

    //----------------------------------------------------------------------------------//

    template <typename Access, typename U, size_t N>
    struct out_of_order;

    template <typename AccessU, size_t N, typename... Access>
    struct out_of_order<std::tuple<Access...>, AccessU, N>
    {
        template <typename Tuple, typename... Args>
        static void access(Tuple&& __t, Args&&... __args)
        {
            TIMEMORY_FOLD_EXPRESSION(
                out_of_order_T<Access, AccessU, N>::template access<Tuple, Args...>(
                    std::forward<Tuple>(__t), std::forward<Args>(__args)...));
        }
    };

    //----------------------------------------------------------------------------------//
    //  invoke a function with a tuple
    //
    template <typename Fn, typename Tuple, size_t... Idx>
    static Ret invoke(Fn&& __f, Tuple&& __t, index_sequence<Idx...>)
    {
        __f(std::get<Idx>(std::forward<Tuple>(__t))...);
    }

    //----------------------------------------------------------------------------------//
    // temporary construction
    //
    template <typename Type, typename... Args>
    static void construct(Args&&... _args)
    {
        Type(std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // temporary construction
    //
    template <typename Type, typename... Args, size_t... Idx>
    static void construct_tuple(std::tuple<Args...>&& _args, index_sequence<Idx>...)
    {
        construct<Type>(std::get<Idx>(_args)...);
    }

    //----------------------------------------------------------------------------------//

    template <template <typename> class Access, typename Tuple, typename... Args,
              size_t... Idx>
    static void unroll_access(Tuple&& __t, index_sequence<Idx...>, Args&&... __args)
    {
        TIMEMORY_FOLD_EXPRESSION(Access<decay_t<decltype(std::get<Idx>(__t))>>(
            std::forward<decltype(std::get<Idx>(__t))>(std::get<Idx>(__t)),
            std::forward<Args>(__args)...));
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Tuple, typename... Args, size_t... Idx>
    static void variadic_1d(Tuple&& __t, Args&&... _args, index_sequence<Idx...>)
    {
        TIMEMORY_FOLD_EXPRESSION(
            construct<typename std::tuple_element<Idx, Access>::type>(
                std::get<Idx>(__t), std::forward<Args>(_args)...));
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename TupleA, typename TupleB, typename... Args,
              size_t... Idx>
    static void variadic_2d(TupleA&& __a, TupleB&& __b, Args&&... _args,
                            index_sequence<Idx...>)
    {
        TIMEMORY_FOLD_EXPRESSION(
            construct<typename std::tuple_element<Idx, Access>::type>(
                std::get<Idx>(__a), std::get<Idx>(__b), std::forward<Args>(_args)...));
    }

    //----------------------------------------------------------------------------------//

    template <template <typename> class Access, typename Tuple, typename... Args,
              size_t... Idx>
    static void type_access(index_sequence<Idx...>, Args&&... __args)
    {
        TIMEMORY_FOLD_EXPRESSION(
            Access<decay_t<typename std::tuple_element<Idx, Tuple>::type>>(
                std::forward<Args>(__args)...));
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Tuple, typename... Args, size_t... Idx>
    static void apply_access_with_indices(Tuple&& __t, index_sequence<Idx...>,
                                          Args&&... __args)
    {
        // call constructor
        TIMEMORY_FOLD_EXPRESSION(decay_t<typename std::tuple_element<Idx, Access>::type>(
            Idx, sizeof...(Idx),
            std::forward<decltype(std::get<Idx>(__t))>(std::get<Idx>(__t)),
            std::forward<Args>(__args)...));
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename TupleA, typename TupleB, typename... Args,
              size_t... Idx>
    static void apply_access2(TupleA&& __ta, TupleB&& __tb, index_sequence<Idx...>,
                              Args&&... __args)
    {
        // call constructor
        TIMEMORY_FOLD_EXPRESSION(decay_t<typename std::tuple_element<Idx, Access>::type>(
            std::forward<decltype(std::get<Idx>(__ta))>(std::get<Idx>(__ta)),
            std::forward<decltype(std::get<Idx>(__tb))>(std::get<Idx>(__tb)),
            std::forward<Args>(__args)...));
    }

    //----------------------------------------------------------------------------------//
    // unroll
    template <size_t N, typename Device, typename Func, typename... Args,
              typename std::enable_if<
                  (N == 1 && std::is_same<Device, device::gpu>::value), char>::type = 0>
    TIMEMORY_LAMBDA static void unroll(Func&& __func, Args&&... __args)
    {
        std::forward<Func>(__func)(std::forward<Args>(__args)...);
    }

    template <size_t N, typename Device, typename Func, typename... Args,
              typename std::enable_if<(N > 1 && std::is_same<Device, device::gpu>::value),
                                      char>::type = 0>
    TIMEMORY_LAMBDA static void unroll(Func&& __func, Args&&... __args)
    {
        std::forward<Func>(__func)(std::forward<Args>(__args)...);
        unroll<N - 1, Device, Func, Args...>(std::forward<Func>(__func),
                                             std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    // unroll
    template <size_t N, typename Device, typename Func, typename... Args,
              typename std::enable_if<
                  (N == 1 && std::is_same<Device, device::cpu>::value), int>::type = 0>
    static void unroll(Func&& __func, Args&&... __args)
    {
        std::forward<Func>(__func)(std::forward<Args>(__args)...);
    }

    template <size_t N, typename Device, typename Func, typename... Args,
              typename std::enable_if<(N > 1 && std::is_same<Device, device::cpu>::value),
                                      int>::type = 0>
    static void unroll(Func&& __func, Args&&... __args)
    {
        std::forward<Func>(__func)(std::forward<Args>(__args)...);
        unroll<N - 1, Device, Func, Args...>(std::forward<Func>(__func),
                                             std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
};

//======================================================================================//

}  // namespace internal

//======================================================================================//
//
//      Declaration
//
//======================================================================================//

template <typename Ret>
struct apply;

//======================================================================================//
//
//          apply --> std::string
//
//======================================================================================//

template <>
struct apply<std::string>
{
    using Ret            = std::string;
    using string_t       = std::string;
    using string_tuple_t = std::tuple<std::string>;

    //----------------------------------------------------------------------------------//
    //      Helper
    //----------------------------------------------------------------------------------//

    template <typename Tp, bool _Val = true, typename Up = int,
              typename Dt = typename std::remove_const<decay_t<Tp>>::type>
    using if_string_t = enable_if_t<(std::is_same<Dt, char*>::value) == _Val, Up>;

    //----------------------------------------------------------------------------------//

    template <typename SepT, typename... Args, typename ReturnT = Ret,
              size_t N = sizeof...(Args), enable_if_t<(N > 0), char> = 0>
    static ReturnT join(SepT&& separator, Args&&... __args) noexcept
    {
        std::stringstream ss;
        ss << std::boolalpha;
        return internal::apply<Ret>::template join<SepT, Args...>(
            std::ref(ss), std::forward<SepT>(separator), std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <typename SepT, typename Arg, if_string_t<Arg, true> = 0>
    static Ret join(SepT&&, Arg&& _arg) noexcept
    {
        return std::move(_arg);
    }

    //----------------------------------------------------------------------------------//

    template <typename SepT, typename Arg, if_string_t<Arg, false> = 0>
    static Ret join(SepT&&, Arg&& _arg) noexcept
    {
        std::stringstream ss;
        ss << _arg;
        return ss.str();
    }

    //----------------------------------------------------------------------------------//

    static Ret join(const string_t&) noexcept { return Ret{ "" }; }

    //----------------------------------------------------------------------------------//
};

//======================================================================================//
//
//          apply --> generic
//
//======================================================================================//

template <typename Ret>
struct apply
{
    using string_t = std::string;

    //----------------------------------------------------------------------------------//
    //  invoke a function
    //
    template <typename Fn, typename... Args, size_t N = sizeof...(Args)>
    static Ret invoke(Fn&& __f, Args&&... __args) noexcept
    {
        return __f(std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    //  invoke a function with a tuple
    //
    template <typename Fn, template <typename...> class Tuple, typename... Args,
              size_t N = sizeof...(Args)>
    static Ret invoke(Fn&& __f, Tuple<Args...>&& __t) noexcept
    {
        using Tuple_t = Tuple<Args...>;
        return internal::apply<Ret>::template invoke<Fn, Tuple_t>(
            std::forward<Fn>(__f), std::forward<Tuple_t>(__t), make_index_sequence<N>{});
    }

    //----------------------------------------------------------------------------------//

    template <typename SepT, typename Tuple, size_t... Idx>
    static string_t join(SepT&& separator, Tuple&& __tup, index_sequence<Idx...>) noexcept
    {
        return apply<string_t>::join(separator, std::get<Idx>(__tup)...);
    }
};

//======================================================================================//
//
//          apply --> std::tuple<std::string>
//
//======================================================================================//

template <>
struct apply<std::tuple<std::string>>
{
    using string_t = std::string;
    using Ret      = string_t;
    using apply_v  = apply<string_t>;

    //----------------------------------------------------------------------------------//
    //  implementation for label + entry join
    //
    struct impl
    {
        template <typename LabelSep, typename EntrySep, typename LabelTup,
                  typename EntryTup, size_t... Idx>
        static Ret join(LabelSep&& _label_sep, EntrySep&& _entry_sep,
                        LabelTup&& _label_tup, EntryTup&& _entry_tup,
                        index_sequence<Idx...>) noexcept
        {
            return apply_v::join(std::forward<LabelSep>(_label_sep),
                                 apply_v::join(std::forward<EntrySep>(_entry_sep),
                                               std::get<Idx>(_label_tup),
                                               std::get<Idx>(_entry_tup))...);
        }
    };

    //----------------------------------------------------------------------------------//
    //  join a tuple of labels with entries
    //
    template <typename LabelSep, typename EntrySep, typename LabelTup, typename EntryTup,
              size_t N                  = std::tuple_size<decay_t<LabelTup>>::value,
              enable_if_t<(N > 0), int> = 0>
    static Ret join(LabelSep&& _label_sep, EntrySep&& _entry_sep, LabelTup&& _label_tup,
                    EntryTup&& _entry_tup) noexcept
    {
        // clang-format off
        return impl::join(std::forward<LabelSep>(_label_sep),
                           std::forward<EntrySep>(_entry_sep),
                           std::forward<LabelTup>(_label_tup),
                           std::forward<EntryTup>(_entry_tup),
                           make_index_sequence<N>{});
        // clang-format on
    }

    //----------------------------------------------------------------------------------//
    //  join a tuple of labels with entries
    //
    template <typename LabelSep, typename EntrySep, typename LabelTup, typename EntryTup,
              size_t N                   = std::tuple_size<decay_t<LabelTup>>::value,
              enable_if_t<(N == 0), int> = 0>
    static Ret join(LabelSep&&, EntrySep&&, LabelTup&&, EntryTup&&) noexcept
    {
        return "";
    }

    //----------------------------------------------------------------------------------//
};

//======================================================================================//
//
//          apply --> void
//
//======================================================================================//

template <>
struct apply<void>
{
    using Ret = void;

    //----------------------------------------------------------------------------------//

    template <size_t I, typename A>
    using Access_t = typename std::tuple_element<I, A>::type;

    //----------------------------------------------------------------------------------//
    //  invoke a function
    //
    template <typename Fn, typename... Args, size_t N = sizeof...(Args)>
    static Ret invoke(Fn&& __f, Args&&... __args) noexcept
    {
        __f(std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    //  invoke a function with a tuple
    //
    template <typename Fn, template <typename...> class Tuple, typename... Args,
              size_t N = sizeof...(Args)>
    static Ret invoke(Fn&& __f, Tuple<Args...>&& __t) noexcept
    {
        using Tuple_t = Tuple<Args...>;
        internal::apply<Ret>::template invoke<Fn, Tuple_t>(
            std::forward<Fn>(__f), std::forward<Tuple_t>(__t), make_index_sequence<N>{});
    }

    //----------------------------------------------------------------------------------//
    //  per-element addition
    //
    template <typename Tuple, size_t N = std::tuple_size<Tuple>::value>
    static void plus(Tuple& _lhs, const Tuple& _rhs) noexcept
    {
        math::plus(_lhs, _rhs);
    }

    //----------------------------------------------------------------------------------//
    //  per-element subtraction
    //
    template <typename Tuple, size_t N = std::tuple_size<Tuple>::value>
    static void minus(Tuple& _lhs, const Tuple& _rhs) noexcept
    {
        math::minus(_lhs, _rhs);
    }

    //----------------------------------------------------------------------------------//
    //  per-element percent difference
    //
    template <typename Tuple, size_t N = std::tuple_size<Tuple>::value>
    static void percent_diff(Tuple& _ret, const Tuple& _lhs, const Tuple& _rhs) noexcept
    {
        _ret = math::compute<Tuple>::percent_diff(_lhs, _rhs);
    }

    //----------------------------------------------------------------------------------//

    template <size_t N, typename Device, typename Func, typename... Args,
              enable_if_t<std::is_same<Device, device::gpu>::value, char> = 0>
    TIMEMORY_LAMBDA static void unroll(Func&& __func, Args&&... __args) noexcept
    {
        internal::apply<void>::template unroll<N, Device, Func, Args...>(
            std::forward<Func>(__func), std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <size_t N, typename Device, typename Func, typename... Args,
              enable_if_t<std::is_same<Device, device::cpu>::value, char> = 0>
    static void unroll(Func&& __func, Args&&... __args) noexcept
    {
        internal::apply<void>::template unroll<N, Device, Func, Args...>(
            std::forward<Func>(__func), std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    //
    //      N > 0
    //
    //----------------------------------------------------------------------------------//

    template <typename Tp, typename Value>
    static inline auto set_value_fold(Tp&& _t, int, Value&& _v) noexcept
        -> decltype(std::forward<Tp>(_t) = std::forward<Value>(_v), void())
    {
        std::forward<Tp>(_t) = std::forward<Value>(_v);
    }

    template <typename Tp, typename Value>
    static inline void set_value_fold(Tp&&, long, Value&&) noexcept
    {}

    template <typename Tuple, typename Value, size_t... Idx>
    static inline void set_value_fold(Tuple&& _t, Value&& _v,
                                      index_sequence<Idx...>) noexcept
    {
        TIMEMORY_FOLD_EXPRESSION(
            set_value_fold(std::get<Idx>(_t), 0, std::forward<Value>(_v)));
    }

    //----------------------------------------------------------------------------------//

    template <typename Tuple, typename Value>
    static inline void set_value(Tuple&& _t, Value&& _v) noexcept
    {
        constexpr auto N = std::tuple_size<decay_t<Tuple>>::value;
        set_value_fold(std::forward<Tuple>(_t), std::forward<Value>(_v),
                       make_index_sequence<N>{});
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Tuple, size_t... Idx, typename... Args>
    static inline void access_fold(Tuple&& _t, index_sequence<Idx...>, Args&&... _args)
    {
        TIMEMORY_FOLD_EXPRESSION(Access_t<Idx, Access>(
            std::forward<decltype(std::get<Idx>(_t))>(std::get<Idx>(_t)),
            std::forward<Args>(_args)...));
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Tuple, typename... Args>
    static inline void access(Tuple&& __t, Args&&... __args) noexcept
    {
        constexpr auto N  = std::tuple_size<decay_t<Access>>::value;
        constexpr auto Nt = std::tuple_size<decay_t<Tuple>>::value;
        static_assert(N == Nt, "Cannot fold Access from Tuple because sizes differ");
        access_fold<Access>(std::forward<Tuple>(__t), std::make_index_sequence<N>{},
                            std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Mapper, size_t R, typename Tuple,
              typename... Args, size_t N = std::tuple_size<decay_t<Access>>::value,
              size_t Nt = std::tuple_size<decay_t<Tuple>>::value,
              enable_if_t<(N > 0 && Nt > 0), int> = 0>
    static void out_of_order(Tuple&& __t, Args&&... __args) noexcept
    {
        using OutOfOrder_t = internal::apply<void>::out_of_order<Access, Mapper, R>;
        OutOfOrder_t::template access<Tuple, Args...>(std::forward<Tuple>(__t),
                                                      std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename Tuple, typename... Args>
    static void access_with_indices(Tuple&& __t, Args&&... __args) noexcept
    {
        constexpr auto N = std::tuple_size<decay_t<Tuple>>::value;
        internal::apply<void>::template apply_access_with_indices<Access, Tuple, Args...>(
            std::forward<Tuple>(__t), make_index_sequence<N>{},
            std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <typename Access, typename TupleA, typename TupleB, typename... Args>
    static void access2(TupleA&& __ta, TupleB&& __tb, Args&&... __args) noexcept
    {
        constexpr size_t N  = std::tuple_size<decay_t<Access>>::value;
        constexpr size_t Na = std::tuple_size<decay_t<TupleA>>::value;
        constexpr size_t Nb = std::tuple_size<decay_t<TupleB>>::value;
        static_assert(Na == Nb, "tuple A size must match tuple B size");
        internal::apply<void>::template apply_access2<Access, TupleA, TupleB, Args...>(
            std::forward<TupleA>(__ta), std::forward<TupleB>(__tb),
            make_index_sequence<N>{}, std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <template <typename> class Access, typename Tuple, typename... Args>
    static void unroll_access(Tuple&& __t, Args&&... __args) noexcept
    {
        constexpr size_t N = std::tuple_size<decay_t<Tuple>>::value;
        internal::apply<void>::template unroll_access<Access, Tuple, Args...>(
            std::forward<Tuple>(__t), make_index_sequence<N>{},
            std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//

    template <template <typename> class Access, typename Tuple, typename... Args>
    static void type_access(Args&&... __args) noexcept
    {
        constexpr size_t N = std::tuple_size<decay_t<Tuple>>::value;
        internal::apply<void>::template type_access<Access, Tuple, Args...>(
            make_index_sequence<N>{}, std::forward<Args>(__args)...);
    }

    //----------------------------------------------------------------------------------//
    //
    //      N == 0
    //
    //----------------------------------------------------------------------------------//

    template <typename Access, typename Mapper, size_t R, typename Tuple,
              typename... Args, size_t N = std::tuple_size<decay_t<Access>>::value,
              size_t Nt = std::tuple_size<decay_t<Tuple>>::value,
              enable_if_t<(N == 0 || Nt == 0), int> = 0>
    static void out_of_order(Tuple&&, Args&&...) noexcept
    {}

    //----------------------------------------------------------------------------------//
};

//======================================================================================//

}  // namespace tim

//======================================================================================//
