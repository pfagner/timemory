#!@PYTHON_EXECUTABLE@
#
# MIT License
#
# Copyright (c) 2018, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of any
# required approvals from the U.S. Dept. of Energy).  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import sys
import copy
from collections import deque
from ..libpytimemory import component_bundle
from ..libpytimemory import settings
from functools import wraps

__all__ = ["profile"]
#
#   Variables
#
_records = deque()
_counter = 0
_skip_counts = []
_start_events = ["call"] # + ["c_call"]
_stop_events = ["return"] # + ["c_return"]
_is_running = False
_components = os.environ.get("TIMEMORY_PROFILER_COMPONENTS", "")
_always_skipped_functions = ["__exit__"]
_always_skipped_files = ["__init__.py"]

#
#   Profiler settings
#
_include_line = True
_include_filepath = True
_full_filepath = False


def _default_functor():
    return True


def _profiler_function(frame, event, arg):

    global _records
    global _include_line
    global _include_filepath
    global _full_filepath
    global _counter
    global _skip_counts
    global _start_events
    global _stop_events
    global _always_skipped_functions

    _count = copy.copy(_counter)

    if event in _start_events:
        if not settings.enabled:
            _skip_counts.append(_count)
            return

        _func = "{}".format(frame.f_code.co_name)        
        _line = int(frame.f_lineno) if _include_line else -1
        _file = "" if not _include_filepath else "{}".format(
            frame.f_code.co_filename)

        # skip anything from this file
        if _file == __file__:
            _skip_counts.append(_count)
            return

        # check if skipped function
        if _func in _always_skipped_functions:
            _skip_counts.append(_count)
            return

        # check if skipped file
        if os.path.basename(_file) in _always_skipped_files:
            _skip_counts.append(_count)
            return

        if not _full_filepath and len(_file) > 0:
            _file = os.path.basename(_file)

        if "__init__.py" not in _file:
            entry = component_bundle(_func, _file, _line)
            entry.start()
            _records.append(entry)
        else:
            _skip_counts.append(_count)
        _counter += 1

    elif event in _stop_events:

        if _count in _skip_counts:
            _skip_counts.remove(_count)
        elif len(_records) > 0:
            entry = _records.pop()
            entry.stop()
            del entry
        _counter -= 1


def _run(sz=[4, 3]):
    import numpy as np
    arr = np.zeros(sz, dtype=np.float)
    arr *= 1.5
    return arr


def _do_sleep(nsec):
    import time
    time.sleep(nsec)


def _main():
    ret = _run()
    print("result: {}".format(ret))
    _do_sleep(1)


#----------------------------------------------------------------------------------------#
#
class profile():
    """
    Provides decorators and context-manager for the timemory profilers
    """
    global _default_functor

    # static variable
    _conditional_functor = _default_functor

    #------------------------------------------------------------------------------------#
    #
    @staticmethod
    def condition(functor):
        profile._conditional_functor = functor

    #------------------------------------------------------------------------------------#
    #
    @staticmethod
    def is_enabled():
        ret = profile._conditional_functor()
        try:
            if ret is True:
                return True
            elif ret is False:
                return False
        except:
            pass
        return False

    #------------------------------------------------------------------------------------#
    #
    def __init__(self, components=[], flat=False, timeline=False, *args, **kwargs):
        """
        Arguments:
            - components [list of strings]  : list of timemory components
            - flat [bool]                   : enable flat profiling
            - timeline [bool]               : enable timeline profiling
        """
        global _records
        global _include_line
        global _include_filepath
        global _full_filepath
        global _counter
        global _skip_counts
        global _start_events
        global _stop_events
        global _is_running
        global _components

        self._original_profiler_function = sys.getprofile()
        self._use = (not _is_running and profile.is_enabled() is True)
        self._flat_profile = (settings.flat_profile or flat)
        self._timeline_profile = (settings.timeline_profile or timeline)
        self.components = components + _components.split(",")
        if len(self.components) == 0:
            self.components += ["wall_clock"]
        os.environ["TIMEMORY_PROFILER_COMPONENTS"] = ",".join(self.components)
        print("USE = {}, COMPONENTS = {}".format(self._use, self.components))

    #------------------------------------------------------------------------------------#
    #
    def start(self):
        """
        Start the profiler explicitly
        """
        global _is_running

        if self._use:
            self._original_profiler_function = sys.getprofile()
            _is_running = True
            component_bundle.reset()
            component_bundle.configure(self.components, self._flat_profile,
                                       self._timeline_profile)
            sys.setprofile(_profiler_function)

    #------------------------------------------------------------------------------------#
    #
    def stop(self):
        """
        Stop the profiler explicitly
        """
        global _is_running

        if self._use:
            _is_running = False
            sys.setprofile(self._original_profiler_function)
            import traceback
            if exec_type is not None and exec_value is not None and exec_tb is not None:
                traceback.print_exception(exec_type, exec_value, exec_tb, limit=5)

    #------------------------------------------------------------------------------------#
    #
    def __call__(self, func):
        """
        Decorator
        """
        global _is_running

        if self._use:
            self._original_profiler_function = sys.getprofile()
            _is_running = True
            component_bundle.reset()
            component_bundle.configure(self.components, self._flat_profile,
                                       self._timeline_profile)

        @wraps(func)
        def function_wrapper(*args, **kwargs):
            if self._use:
                sys.setprofile(_profiler_function)
            _ret = func(*args, **kwargs)
            if self._use:
                sys.setprofile(self._original_profiler_function)
            return _ret

        _ret = function_wrapper

        if self._use:
            _is_running = False

        return _ret

    #------------------------------------------------------------------------------------#
    #
    def __enter__(self, *args, **kwargs):
        """
        Context manager
        """
        global _is_running

        if self._use:
            self._original_profiler_function = sys.getprofile()
            _is_running = True
            component_bundle.reset()
            component_bundle.configure(self.components, self._flat_profile,
                                       self._timeline_profile)
            sys.setprofile(_profiler_function)

    #------------------------------------------------------------------------------------#
    #
    def __exit__(self, exec_type, exec_value, exec_tb):
        """
        Context manager
        """
        global _is_running

        if self._use:
            _is_running = False
            sys.setprofile(self._original_profiler_function)

        import traceback
        if exec_type is not None and exec_value is not None and exec_tb is not None:
            traceback.print_exception(exec_type, exec_value, exec_tb, limit=5)

    #------------------------------------------------------------------------------------#
    #
    def run(self, cmd):
        import __main__
        dict = __main__.__dict__
        if isinstance(cmd, str):
            return self.runctx(cmd, dict, dict)
        else:
            return self.runctx(" ".join(cmd), dict, dict)

    #------------------------------------------------------------------------------------#
    #
    def runctx(self, cmd, globals, locals):
        global _is_running

        if self._use:
            self._original_profiler_function = sys.getprofile()
            _is_running = True
            component_bundle.reset()
            component_bundle.configure(self.components, self._flat_profile,
                                       self._timeline_profile)

        try:
            exec(cmd, globals, locals)
        finally:
            if self._use:
                _is_running = False
        return self
