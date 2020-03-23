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
 * \file timemory/components/ompt/types.hpp
 * \brief Declare the ompt component types
 */

#pragma once

#include "timemory/components/macros.hpp"

//======================================================================================//
//
// TIMEMORY_DECLARE_COMPONENT(example)
//
//======================================================================================//

#include "timemory/components/ompt/properties.hpp"
#include "timemory/components/ompt/traits.hpp"

//
//--------------------------------------------------------------------------------------//
//
#if !defined(TIMEMORY_OMPT_LINKAGE)
//
#    if defined(TIMEMORY_OMPT_SOURCE)
//
#        define TIMEMORY_OMPT_LINKAGE(...) extern "C" __VA_ARGS__
//
#    elif defined(TIMEMORY_USE_EXTERN) || defined(TIMEMORY_USE_OMPT_EXTERN)
//
#        define TIMEMORY_OMPT_LINKAGE(...) extern "C" __VA_ARGS__
//
#    else
//
#        define TIMEMORY_OMPT_LINKAGE(...) extern "C" __VA_ARGS__
//
#    endif
//
#endif
//
//--------------------------------------------------------------------------------------//
//
#if defined(TIMEMORY_USE_OMPT)

#    include <omp.h>
#    include <ompt.h>

//--------------------------------------------------------------------------------------//
//
TIMEMORY_OMPT_LINKAGE(int)
ompt_initialize(ompt_function_lookup_t lookup, ompt_data_t* tool_data);
//
TIMEMORY_OMPT_LINKAGE(ompt_start_tool_result_t*)
ompt_start_tool(unsigned int omp_version, const char* runtime_version);
//
TIMEMORY_OMPT_LINKAGE(void)
ompt_finalize(ompt_data_t* tool_data);
//
//--------------------------------------------------------------------------------------//

#endif
