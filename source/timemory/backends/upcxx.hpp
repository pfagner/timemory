// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/** \file backends/upcxx.hpp
 * \headerfile backends/upcxx.hpp "timemory/backends/upcxx.hpp"
 * Defines UPC++ backend
 *
 */

#pragma once

#include "timemory/utility/macros.hpp"  // macro definitions w/ no internal deps
#include "timemory/utility/types.hpp"
#include "timemory/utility/utility.hpp"  // generic functions w/ no internal deps

#include <cstdint>
#include <future>
#include <unordered_map>

#if defined(TIMEMORY_USE_UPCXX)
#    include <upcxx/upcxx.hpp>
#endif

namespace tim
{
/// use upc as the namespace instead of upcxx to ensure there are no namespace ambiguities
/// plus, 'upc' has same number of characters as 'mpi'
namespace upc
{
#if defined(TIMEMORY_USE_UPCXX)
using comm_t = ::upcxx::team;
template <typename _Tp>
using future_t = ::upcxx::future<_Tp>;
template <typename _Tp>
using promise_t = ::upcxx::promise<_Tp>;
inline comm_t&
world()
{
    return ::upcxx::world();
}
#else
using comm_t = int32_t;
template <typename _Tp>
using future_t = ::std::future<_Tp>;
template <typename _Tp>
using promise_t = ::std::promise<_Tp>;
inline comm_t&
world()
{
    static comm_t _instance = 0;
    return _instance;
}
#endif

//--------------------------------------------------------------------------------------//

inline void
barrier(comm_t& comm = world());

template <typename... _Args>
inline int32_t
rank(_Args&&...);

template <typename... _Args>
inline int32_t
size(_Args&&...);

//--------------------------------------------------------------------------------------//

inline bool
is_supported()
{
#if defined(TIMEMORY_USE_UPCXX)
    return true;
#else
    return false;
#endif
}

//--------------------------------------------------------------------------------------//

inline bool&
is_finalized()
{
#if defined(TIMEMORY_USE_UPCXX)
    static bool _instance = false;
#else
    static bool _instance = true;
#endif
    return _instance;
}

//--------------------------------------------------------------------------------------//

inline bool
is_initialized()
{
#if defined(TIMEMORY_USE_UPCXX)
    if(!is_finalized())
        return ::upcxx::initialized();
#endif
    return false;
}

//--------------------------------------------------------------------------------------//

template <typename... _Args>
inline void
initialize(_Args&&...)
{
#if defined(TIMEMORY_USE_UPCXX)
    if(!is_initialized())
        ::upcxx::init();
#endif
}

//--------------------------------------------------------------------------------------//

inline void
finalize()
{
#if defined(TIMEMORY_USE_UPCXX)
    if(is_initialized())
    {
        ::upcxx::finalize();
        is_finalized() = true;
    }
#endif
}

//--------------------------------------------------------------------------------------//

template <typename... _Args>
inline int32_t
rank(_Args&&...)
{
#if defined(TIMEMORY_USE_UPCXX)
    if(is_initialized())
        return ::upcxx::rank_me();
#endif
    return 0;
}

//--------------------------------------------------------------------------------------//

template <typename... _Args>
inline int32_t
size(_Args&&...)
{
#if defined(TIMEMORY_USE_UPCXX)
    if(is_initialized())
        return ::upcxx::rank_n();
#endif
    return 1;
}

//--------------------------------------------------------------------------------------//

inline void
barrier(comm_t& comm)
{
#if defined(TIMEMORY_USE_UPCXX)
    if(is_initialized())
        ::upcxx::barrier(comm);
#else
    consume_parameters(comm);
#endif
}

//--------------------------------------------------------------------------------------//

}  // namespace upc
}  // namespace tim
