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
 * \file timemory/components/gperftools/types.hpp
 * \brief Declare the gperftools component types
 */

#pragma once

#include "timemory/components/macros.hpp"

//======================================================================================//
//
TIMEMORY_DECLARE_COMPONENT(gperftools_cpu_profiler)
TIMEMORY_DECLARE_COMPONENT(gperftools_heap_profiler)
//
// for backwards-compatibility -- name change due to confusion with "gperf" (GNU perf)
//
TIMEMORY_COMPONENT_ALIAS(gperf_cpu_profiler, gperftools_cpu_profiler)
TIMEMORY_COMPONENT_ALIAS(gperf_heap_profiler, gperftools_heap_profiler)
//
//======================================================================================//

#include "timemory/components/gperftools/properties.hpp"
#include "timemory/components/gperftools/traits.hpp"
