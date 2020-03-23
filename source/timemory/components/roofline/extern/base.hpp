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

/**
 * \file timemory/components/roofline/extern/base.hpp
 * \brief Declare the extern base instances for roofline components
 */

#pragma once

#include "timemory/components/base.hpp"
#include "timemory/components/roofline/components.hpp"
#include "timemory/components/roofline/traits.hpp"
#include "timemory/components/roofline/types.hpp"

//======================================================================================//
//
namespace tim
{
namespace component
{
//
#if defined(TIMEMORY_USE_PAPI_EXTERN)
//
TIMEMORY_EXTERN_TEMPLATE(
    struct base<cpu_roofline<float>, std::pair<std::vector<long long>, double>>)
//
TIMEMORY_EXTERN_TEMPLATE(
    struct base<cpu_roofline<double>, std::pair<std::vector<long long>, double>>)
//
TIMEMORY_EXTERN_TEMPLATE(
    struct base<cpu_roofline<float, double>, std::pair<std::vector<long long>, double>>)
//
#endif
//
//--------------------------------------------------------------------------------------//
//
#if defined(TIMEMORY_USE_CUPTI_EXTERN)
//
TIMEMORY_EXTERN_TEMPLATE(struct base<gpu_roofline<cuda::fp16_t, float, double>,
                                     std::tuple<typename cupti_activity::value_type,
                                                typename cupti_counters::value_type>>)
//
TIMEMORY_EXTERN_TEMPLATE(struct base<gpu_roofline<cuda::fp16_t>,
                                     std::tuple<typename cupti_activity::value_type,
                                                typename cupti_counters::value_type>>)
//
TIMEMORY_EXTERN_TEMPLATE(
    struct base<gpu_roofline<float>, std::tuple<typename cupti_activity::value_type,
                                                typename cupti_counters::value_type>>)
//
TIMEMORY_EXTERN_TEMPLATE(
    struct base<gpu_roofline<double>, std::tuple<typename cupti_activity::value_type,
                                                 typename cupti_counters::value_type>>)
//
#endif
//
}  // namespace component
}  // namespace tim
//
//======================================================================================//

#include "timemory/components/roofline/components.hpp"

//======================================================================================//
//
namespace tim
{
namespace component
{
//
#if defined(TIMEMORY_USE_PAPI_EXTERN)
//
TIMEMORY_EXTERN_TEMPLATE(struct cpu_roofline<float, double>)
//
TIMEMORY_EXTERN_TEMPLATE(struct cpu_roofline<float>)
//
TIMEMORY_EXTERN_TEMPLATE(struct cpu_roofline<double>)
//
#endif
//
//--------------------------------------------------------------------------------------//
//
#if defined(TIMEMORY_USE_CUPTI_EXTERN)
//
TIMEMORY_EXTERN_TEMPLATE(struct gpu_roofline<cuda::fp16_t, float, double>)
//
TIMEMORY_EXTERN_TEMPLATE(struct gpu_roofline<cuda::fp16_t>)
//
TIMEMORY_EXTERN_TEMPLATE(struct gpu_roofline<float>)
//
TIMEMORY_EXTERN_TEMPLATE(struct gpu_roofline<double>)
//
#endif
//
}  // namespace component
}  // namespace tim
//
//======================================================================================//
