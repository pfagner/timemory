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
 * \file timemory/components/vtune/extern.hpp
 * \brief Include the extern declarations for vtune components
 */

#pragma once

#if defined(TIMEMORY_USE_VTUNE)

#    include "timemory/components/base.hpp"
#    include "timemory/components/macros.hpp"
//
#    include "timemory/components/vtune/components.hpp"
#    include "timemory/components/vtune/types.hpp"
//
#    if defined(TIMEMORY_COMPONENT_SOURCE) ||                                            \
        (!defined(TIMEMORY_USE_EXTERN) && !defined(TIMEMORY_USE_COMPONENT_EXTERN))
// source/header-only requirements
#        include "timemory/environment/declaration.hpp"
#        include "timemory/operations/definition.hpp"
#        include "timemory/plotting/definition.hpp"
#        include "timemory/settings/declaration.hpp"
#        include "timemory/storage/definition.hpp"
#    else
// extern requirements
#        include "timemory/environment/declaration.hpp"
#        include "timemory/operations/definition.hpp"
#        include "timemory/plotting/declaration.hpp"
#        include "timemory/settings/declaration.hpp"
#        include "timemory/storage/declaration.hpp"
#    endif
//
//======================================================================================//
//
namespace tim
{
namespace component
{
//
TIMEMORY_EXTERN_TEMPLATE(struct base<vtune_event, void>)
TIMEMORY_EXTERN_TEMPLATE(struct base<vtune_frame, void>)
TIMEMORY_EXTERN_TEMPLATE(struct base<vtune_profiler, void>)
//
}  // namespace component
}  // namespace tim
//
//======================================================================================//
//
TIMEMORY_EXTERN_OPERATIONS(component::vtune_event, false)
TIMEMORY_EXTERN_OPERATIONS(component::vtune_frame, false)
TIMEMORY_EXTERN_OPERATIONS(component::vtune_profiler, false)
//
//======================================================================================//
//
TIMEMORY_EXTERN_STORAGE(component::vtune_event, vtune_event)
TIMEMORY_EXTERN_STORAGE(component::vtune_frame, vtune_frame)
TIMEMORY_EXTERN_STORAGE(component::vtune_profiler, vtune_profiler)
//
//======================================================================================//

#endif  // TIMEMORY_USE_VTUNE
