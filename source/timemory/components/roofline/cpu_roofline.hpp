//  MIT License
//
//  Copyright (c) 2020, The Regents of the University of California,
//  through Lawrence Berkeley National Laboratory (subject to receipt of any
//  required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

#pragma once

#include "timemory/backends/papi.hpp"
#include "timemory/components/base.hpp"
#include "timemory/components/macros.hpp"
#include "timemory/settings/declaration.hpp"

#include "timemory/components/roofline/backends.hpp"
#include "timemory/components/roofline/types.hpp"

#include <array>
#include <memory>
#include <numeric>
#include <utility>

//======================================================================================//

namespace tim
{
namespace component
{
//--------------------------------------------------------------------------------------//
// this computes the numerator of the roofline for a given set of PAPI counters.
// e.g. for FLOPS roofline (floating point operations / second:
//
//  single precision:
//              cpu_roofline<float>
//
//  double precision:
//              cpu_roofline<double>
//
//  generic:
//              cpu_roofline<T, ...>
//
template <typename... _Types>
struct cpu_roofline
: public base<cpu_roofline<_Types...>, std::pair<std::vector<long long>, double>>
{
    static_assert(!is_one_of<cuda::fp16_t, std::tuple<_Types...>>::value,
                  "Error! No CPU roofline support for cuda::fp16_t");

    using size_type    = std::size_t;
    using event_type   = std::vector<int>;
    using array_type   = std::vector<long long>;
    using data_type    = long long*;
    using value_type   = std::pair<array_type, double>;
    using this_type    = cpu_roofline<_Types...>;
    using base_type    = base<this_type, value_type>;
    using storage_type = typename base_type::storage_type;
    using record_type  = std::function<value_type()>;

    using unit_type         = typename trait::units<this_type>::type;
    using display_unit_type = typename trait::units<this_type>::display_type;

    using device_t    = device::cpu;
    using count_type  = wall_clock;
    using ratio_t     = typename count_type::ratio_t;
    using types_tuple = std::tuple<_Types...>;

    using ert_data_t     = ert::exec_data<count_type>;
    using ert_data_ptr_t = std::shared_ptr<ert_data_t>;

    // short-hand for variadic expansion
    template <typename _Tp>
    using ert_config_type = ert::configuration<device_t, _Tp, count_type>;
    template <typename _Tp>
    using ert_counter_type = ert::counter<device_t, _Tp, count_type>;
    template <typename _Tp>
    using ert_executor_type = ert::executor<device_t, _Tp, count_type>;
    template <typename _Tp>
    using ert_callback_type = ert::callback<ert_executor_type<_Tp>>;

    // variadic expansion for ERT types
    using ert_config_t   = std::tuple<ert_config_type<_Types>...>;
    using ert_counter_t  = std::tuple<ert_counter_type<_Types>...>;
    using ert_executor_t = std::tuple<ert_executor_type<_Types>...>;
    using ert_callback_t = std::tuple<ert_callback_type<_Types>...>;

    static_assert(std::tuple_size<ert_config_t>::value ==
                      std::tuple_size<types_tuple>::value,
                  "Error! ert_config_t size does not match types_tuple size!");

    using iterator       = typename array_type::iterator;
    using const_iterator = typename array_type::const_iterator;

    static const short precision = 3;
    static const short width     = 8;

    //----------------------------------------------------------------------------------//

    // collection mode, AI (arithmetic intensity) is the load/store: PAPI_LST_INS
    enum class MODE
    {
        OP,
        AI
    };

    //----------------------------------------------------------------------------------//

    using strvec_t          = std::vector<std::string>;
    using intvec_t          = std::vector<int>;
    using events_callback_t = std::function<intvec_t(const MODE&)>;

    //----------------------------------------------------------------------------------//
    /// replace this callback to add in custom HW counters
    static events_callback_t& get_events_callback()
    {
        static events_callback_t _instance = [](const MODE&) { return intvec_t{}; };
        return _instance;
    }

    //----------------------------------------------------------------------------------//
    /// set to false to suppress adding predefined enumerations
    static bool& use_predefined_enums()
    {
        static bool _instance = true;
        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static int event_set() { return private_event_set(); }

    //----------------------------------------------------------------------------------//

    static event_type events()
    {
        return (private_events()) ? (*private_events()) : event_type{};
    }

    //----------------------------------------------------------------------------------//

    static size_type size()
    {
        return (private_events()) ? (private_events()->size()) : 0;
    }

    //----------------------------------------------------------------------------------//

    static MODE& event_mode()
    {
        auto aslc = [](std::string str) {
            for(auto& itr : str)
                itr = tolower(itr);
            return str;
        };

        auto _get = [=]() {
            // check the standard variable
            std::string _env = aslc(settings::cpu_roofline_mode());
            if(_env.empty())
                _env = aslc(settings::roofline_mode());
            return (_env == "op" || _env == "hw" || _env == "counters")
                       ? MODE::OP
                       : ((_env == "ai" || _env == "ac" || _env == "activity")
                              ? MODE::AI
                              : MODE::OP);
        };

        static MODE _instance = _get();
        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static ert_config_t& get_finalizer()
    {
        static ert_config_t _instance;
        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static ert_data_ptr_t get_ert_data()
    {
        static ert_data_ptr_t _instance = std::make_shared<ert_data_t>();
        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static bool initialize_papi()
    {
        static thread_local bool _initalized = false;
        static thread_local bool _working    = false;
        if(!_initalized)
        {
            papi::init();
            papi::register_thread();
            _initalized = true;
            _working    = papi::working();
            if(!_working)
            {
                std::cerr << "Warning! PAPI failed to initialized!\n";
                std::cerr << std::flush;
            }
        }
        return _working;
    }

    //----------------------------------------------------------------------------------//

    static event_type get_events()
    {
        static auto _instance = []() {
            event_type _events;
            if(event_mode() == MODE::OP)
            {
                //
                // add in user callback events BEFORE presets based on type so that
                // the user can override the counters being used
                //
                auto _extra_events = get_events_callback()(event_mode());
                for(const auto& itr : _extra_events)
                    _events.push_back(itr);

                //
                //  add some presets based on data types
                //
                if(use_predefined_enums())
                {
                    if(is_one_of<float, types_tuple>::value)
                        _events.push_back(PAPI_SP_OPS);
                    if(is_one_of<double, types_tuple>::value)
                        _events.push_back(PAPI_DP_OPS);
                }
            }
            else if(event_mode() == MODE::AI)
            {
                //
                //  add the load/store hardware counter
                //
                if(use_predefined_enums())
                {
                    _events.push_back(PAPI_LD_INS);
                    _events.push_back(PAPI_SR_INS);
                    _events.push_back(PAPI_LST_INS);
                    _events.push_back(PAPI_TOT_INS);
                }
                //
                // add in user callback events AFTER load/store so that load/store
                // instructions are always counted
                //
                auto _extra_events = get_events_callback()(event_mode());
                for(const auto& itr : _extra_events)
                    _events.push_back(itr);
            }

            auto array_events = papi_array_t::get_initializer()();
            for(auto& itr : array_events)
            {
                if(std::find(_events.begin(), _events.end(), itr) == _events.end())
                    _events.push_back(itr);
            }

            return _events;
        }();

        if(private_events()->empty())
            *private_events() = _instance;

        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static void thread_init(storage_type*)
    {
        if(!initialize_papi())
            return;

        static thread_local bool _first = true;
        if(!_first)
            return;
        _first = true;

        // create the hardware counter events to accumulate
        event_type _events = get_events();

        // found that PAPI occassionally seg-faults during add_event...
        static std::mutex            _mutex;
        std::unique_lock<std::mutex> _lock(_mutex);

        papi::create_event_set(&private_event_set(), settings::papi_multiplexing());
        if(event_set() == PAPI_NULL)
        {
            fprintf(stderr, "[cpu_roofline]> event_set is PAPI_NULL!\n");
        }
        else
        {
            for(auto itr : _events)
            {
                if(papi::add_event(event_set(), itr))
                {
                    if(settings::verbose() > 1 || settings::debug())
                        printf("[cpu_roofline]> Added event %s to event set %i\n",
                               papi::get_event_code_name(itr).c_str(), event_set());
                }
                else
                {
                    auto pitr = std::find(private_events()->begin(),
                                          private_events()->end(), itr);
                    if(pitr != private_events()->end())
                        private_events()->erase(pitr);

                    if(!settings::papi_quiet())
                        fprintf(stderr,
                                "[cpu_roofline]> Failed to add event %s to event "
                                "set %i\n",
                                papi::get_event_code_name(itr).c_str(), event_set());
                }
            }
        }

        if(private_events()->size() == 0)
            throw std::runtime_error("No events for the roofline!");
        if(private_events()->size() != events().size())
            throw std::runtime_error("Mismatched!");

        if(private_events()->size() > 0)
            papi::start(event_set());
    }

    //----------------------------------------------------------------------------------//

    static void thread_finalize(storage_type*)
    {
        // found that PAPI occassionally seg-faults during add_event so adding here too...
        static std::mutex            _mutex;
        std::unique_lock<std::mutex> _lock(_mutex);

        if(event_set() != PAPI_NULL && events().size() > 0)
        {
            // stop PAPI counters
            array_type event_values(events().size(), 0);
            papi::stop(event_set(), event_values.data());
            papi::remove_events(event_set(), private_events()->data(), events().size());
            papi::destroy_event_set(event_set());
        }
        private_event_set() = PAPI_NULL;
        papi::unregister_thread();
    }

    //----------------------------------------------------------------------------------//

    template <typename _Tp, typename _Func>
    static void set_executor_callback(_Func&& f)
    {
        ert_executor_type<_Tp>::get_callback() = std::forward<_Func>(f);
    }

    //----------------------------------------------------------------------------------//

    static void global_finalize(storage_type* _store)
    {
        if(_store && _store->size() > 0)
        {
            // run roofline peak generation
            auto ert_config = get_finalizer();
            auto ert_data   = get_ert_data();
            apply<void>::access<ert_executor_t>(ert_config, ert_data);
            if(ert_data && (settings::verbose() > 0 || settings::debug()))
                std::cout << *(ert_data) << std::endl;
        }
    }

    //----------------------------------------------------------------------------------//

    template <typename _Archive>
    static void extra_serialization(_Archive& ar, const unsigned int /*version*/)
    {
        auto _ert_data = get_ert_data();
        if(!_ert_data.get())  // for input
            _ert_data.reset(new ert_data_t());
        ar(cereal::make_nvp("roofline", *_ert_data.get()));
    }

    //----------------------------------------------------------------------------------//

    static std::string get_mode_string()
    {
        return (event_mode() == MODE::OP) ? "op" : "ai";
    }

    //----------------------------------------------------------------------------------//

    static std::string get_type_string()
    {
        return apply<std::string>::join("_", demangle(typeid(_Types).name())...);
    }

    //----------------------------------------------------------------------------------//

    static unit_type unit()
    {
        return (event_mode() == MODE::OP) ? (1.0 / count_type::unit()) : 1.0;
    }

    //----------------------------------------------------------------------------------//

    static display_unit_type display_unit()
    {
        display_unit_type _units{};
        auto              labels = events_label_array();
        for(size_type i = 0; i < labels.size(); ++i)
        {
            std::stringstream ss;
            ss << labels[i];
            if(ss.str().length() == 0)
            {
                _units.push_back("");
            }
            else
            {
                if(event_mode() == MODE::OP)
                    ss << " / " << count_type::display_unit();
                _units.push_back(ss.str());
            }
        }

        return _units;
    }

    //----------------------------------------------------------------------------------//

    static unit_type get_unit() { return unit(); }

    //----------------------------------------------------------------------------------//

    static display_unit_type get_display_unit() { return display_unit(); }

    //----------------------------------------------------------------------------------//

    static std::string label()
    {
        if(settings::roofline_type_labels_cpu() || settings::roofline_type_labels())
            return std::string("cpu_roofline_") + get_type_string() + "_" +
                   get_mode_string();
        else
            return std::string("cpu_roofline_") + get_mode_string();
    }

    //----------------------------------------------------------------------------------//

    static std::string description()
    {
        return "CPU Roofline " + get_type_string() + " " +
               std::string((event_mode() == MODE::OP) ? "Counters"
                                                      : "Arithmetic Intensity");
    }

    //----------------------------------------------------------------------------------//

    static value_type record()
    {
        array_type read_values(size(), 0);
        papi::read(event_set(), read_values.data());
        auto delta_duration =
            count_type::record() / static_cast<double>(ratio_t::den) * units::sec;
        return value_type(read_values, delta_duration);
    }

public:
    //----------------------------------------------------------------------------------//

    cpu_roofline()
    : m_events(get_events())
    , m_record([]() { return this_type::record(); })
    {
        resize(m_events.size());
        std::tie(value.second, accum.second) = std::make_pair(0, 0);
    }

    //----------------------------------------------------------------------------------//

    // ~cpu_roofline()                       = default;
    // cpu_roofline(const cpu_roofline& rhs) = default;
    // cpu_roofline(cpu_roofline&& rhs)      = default;
    // this_type& operator=(const this_type&) = default;
    // this_type& operator=(this_type&&) = default;

    //----------------------------------------------------------------------------------//

    std::vector<double> get() const
    {
        auto                _n = size() + 1;
        std::vector<double> _data(_n, 0.0);
        const auto&         obj = (is_transient) ? accum : value;

        for(size_t i = 0; i < obj.first.size(); ++i)
            _data[i] += static_cast<double>(obj.first[i]);

        _data.back() += obj.second;

        return _data;
    }

    //----------------------------------------------------------------------------------//

    void start()
    {
        set_started();
        value = m_record();
    }

    //----------------------------------------------------------------------------------//

    void stop()
    {
        auto tmp = m_record();
        resize(std::max<size_type>(tmp.first.size(), value.first.size()));
        for(size_type i = 0; i < accum.first.size(); ++i)
            accum.first[i] += (tmp.first[i] - value.first[i]);
        accum.second += (tmp.second - value.second);
        value = std::move(tmp);
        set_stopped();
    }

    //----------------------------------------------------------------------------------//

    template <typename _Func>
    void configure_record(_Func&& _func)
    {
        m_record = std::forward<_Func>(_func);
    }

    //----------------------------------------------------------------------------------//

    template <typename _Func>
    void configure_record(const MODE& _mode, _Func&& _func)
    {
        if(event_mode() == _mode)
            m_record = std::forward<_Func>(_func);
    }

    //----------------------------------------------------------------------------------//

    this_type& operator+=(const this_type& rhs)
    {
        resize(std::max<size_type>(m_events.size(), events().size()));
        for(size_type i = 0; i < accum.first.size(); ++i)
            accum.first[i] += rhs.accum.first[i];
        for(size_type i = 0; i < value.first.size(); ++i)
            value.first[i] += rhs.value.first[i];
        accum.second += rhs.accum.second;
        value.second += rhs.value.second;
        if(rhs.is_transient)
            is_transient = rhs.is_transient;
        return *this;
    }

    //----------------------------------------------------------------------------------//

    this_type& operator-=(const this_type& rhs)
    {
        resize(std::max<size_type>(m_events.size(), events().size()));
        for(size_type i = 0; i < accum.first.size(); ++i)
            accum.first[i] -= rhs.accum.first[i];
        for(size_type i = 0; i < value.first.size(); ++i)
            value.first[i] -= rhs.value.first[i];
        accum.second -= rhs.accum.second;
        value.second -= rhs.value.second;
        if(rhs.is_transient)
            is_transient = rhs.is_transient;
        return *this;
    }

    //----------------------------------------------------------------------------------//

    iterator begin()
    {
        auto& obj = (accum.second > 0) ? accum.first : value.first;
        return obj.begin();
    }

    //----------------------------------------------------------------------------------//

    const_iterator begin() const
    {
        const auto& obj = (accum.second > 0) ? accum.first : value.first;
        return obj.begin();
    }

    //----------------------------------------------------------------------------------//

    iterator end()
    {
        auto& obj = (accum.second > 0) ? accum.first : value.first;
        return obj.end();
    }

    //----------------------------------------------------------------------------------//

    const_iterator end() const
    {
        const auto& obj = (accum.second > 0) ? accum.first : value.first;
        return obj.end();
    }

    //----------------------------------------------------------------------------------//

    double get_elapsed(const int64_t& _unit = count_type::get_unit()) const
    {
        auto& obj = (accum.second > 0) ? accum : value;
        return static_cast<double>(obj.second) *
               (static_cast<double>(_unit) / units::sec);
    }

    //----------------------------------------------------------------------------------//

    double get_counted() const
    {
        double _sum = 0.0;
        for(auto itr = begin(); itr != end(); ++itr)
            _sum += static_cast<double>(*itr);
        return _sum;
    }

protected:
    using base_type::accum;
    using base_type::is_transient;
    using base_type::laps;
    using base_type::set_started;
    using base_type::set_stopped;
    using base_type::value;

    friend struct base<this_type, value_type>;

    using base_type::implements_storage_v;
    friend class impl::storage<this_type, implements_storage_v>;

public:
    //==================================================================================//
    //
    //      representation as a string
    //
    //==================================================================================//

    std::vector<double> get_display() const { return get(); }

    //----------------------------------------------------------------------------------//

    friend std::ostream& operator<<(std::ostream& os, const this_type& obj)
    {
        using namespace tim::stl::ostream;

        // output the time
        auto&             _obj = (obj.accum.second > 0) ? obj.accum : obj.value;
        std::stringstream sst;
        auto              t_value = _obj.second;
        auto              t_label = count_type::get_label();
        auto              t_disp  = count_type::get_display_unit();
        auto              t_prec  = count_type::get_precision();
        auto              t_width = count_type::get_width();
        auto              t_flags = count_type::get_format_flags();

        sst.setf(t_flags);
        sst << std::setw(t_width) << std::setprecision(t_prec) << t_value;
        if(!t_disp.empty())
            sst << " " << t_disp;
        if(!t_label.empty())
            sst << " " << t_label;
        sst << ", ";

        auto _prec  = count_type::get_precision();
        auto _width = this_type::get_width();
        auto _flags = count_type::get_format_flags();

        // output the roofline metric
        auto _value = obj.get();
        auto _label = obj.label_array();
        auto _disp  = obj.display_unit_array();

#if defined(DEBUG)
        if(settings::debug())
        {
            std::cout << "value: " << _value << std::endl;
            std::cout << "label: " << _label << std::endl;
            std::cout << "displ: " << _disp << std::endl;
        }
#endif
        assert(_value.size() <= _label.size());
        assert(_value.size() <= _disp.size());

        auto n = _label.size();
        for(size_t i = 0; i < n; ++i)
        {
            std::stringstream ss_value;
            std::stringstream ss_extra;
            ss_value.setf(_flags);
            ss_value << std::setw(_width) << std::setprecision(_prec) << _value.at(i);
            if(!_disp.at(i).empty())
                ss_extra << " " << _disp.at(i);
            else if(!_label.at(i).empty())
                ss_extra << " " << _label.at(i);
            os << sst.str() << ss_value.str() << ss_extra.str();
            if(i + 1 < n)
                os << ", ";
        }

        return os;
    }

    //----------------------------------------------------------------------------------//

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int)
    {
        auto _disp = get_display();

        ar(cereal::make_nvp("is_transient", is_transient), cereal::make_nvp("laps", laps),
           cereal::make_nvp("display", _disp),
           cereal::make_nvp("mode", get_mode_string()),
           cereal::make_nvp("type", get_type_string()));

        const auto& labels = events_label_array();
        auto        data   = get();
        ar.setNextName("repr_data");
        ar.startNode();
        auto litr = labels.begin();
        auto ditr = data.begin();
        for(; litr != labels.end() && ditr != data.end(); ++litr, ++ditr)
            ar(cereal::make_nvp(*litr, double(*ditr)));
        ar.finishNode();

        ar(cereal::make_nvp("value", value), cereal::make_nvp("accum", accum),
           cereal::make_nvp("units", unit_array()),
           cereal::make_nvp("display_units", display_unit_array()));
    }

    //----------------------------------------------------------------------------------//
    // array of descriptions
    //
    strvec_t label_array() const
    {
        strvec_t arr;
        for(const auto& itr : m_events)
            arr.push_back(papi::get_event_info(itr).short_descr);
        arr.push_back("Runtime");

        for(auto& itr : arr)
        {
            size_t n = std::string::npos;
            while((n = itr.find("L/S")) != std::string::npos)
                itr.replace(n, 3, "Loads_Stores");
        }

        for(auto& itr : arr)
        {
            size_t n = std::string::npos;
            while((n = itr.find("/")) != std::string::npos)
                itr.replace(n, 1, "_per_");
        }

        for(auto& itr : arr)
        {
            size_t n = std::string::npos;
            while((n = itr.find(" ")) != std::string::npos)
                itr.replace(n, 1, "_");

            while((n = itr.find("__")) != std::string::npos)
                itr.replace(n, 2, "_");
        }

        if(events_label_array().size() < arr.size())
            events_label_array() = arr;
        return arr;
    }

    //----------------------------------------------------------------------------------//
    // array of labels
    //
    strvec_t description_array() const
    {
        std::vector<std::string> arr(m_events.size());
        for(size_type i = 0; i < m_events.size(); ++i)
            arr[i] = papi::get_event_info(m_events[i]).long_descr;
        arr.push_back("Runtime");
        return arr;
    }

    //----------------------------------------------------------------------------------//
    //
    strvec_t display_unit_array() const
    {
        strvec_t arr;
        for(const auto& itr : m_events)
            arr.push_back(papi::get_event_info(itr).units);
        arr.push_back(count_type::get_display_unit());
        return arr;
    }

    //----------------------------------------------------------------------------------//
    // array of unit values
    //
    std::vector<int64_t> unit_array() const
    {
        std::vector<int64_t> arr(m_events.size() + 1, 1);
        arr.back() = count_type::get_unit();
        return arr;
    }

private:
    //----------------------------------------------------------------------------------//
    // these are needed after the global label array is destroyed
    //
    event_type  m_events = get_events();
    record_type m_record = []() { return this_type::record(); };

    //----------------------------------------------------------------------------------//

    void resize(size_type sz)
    {
        sz = std::max<size_type>(size(), sz);
        value.first.resize(std::max<size_type>(sz, value.first.size()), 0);
        accum.first.resize(std::max<size_type>(sz, accum.first.size()), 0);
    }

private:
    //----------------------------------------------------------------------------------//

    static strvec_t& events_label_array()
    {
        static thread_local strvec_t _instance{};
        return _instance;
    }

    //----------------------------------------------------------------------------------//

    static event_type* private_events()
    {
        using pointer_t                    = std::unique_ptr<event_type>;
        static thread_local auto _instance = pointer_t(new event_type{});
        return _instance.get();
    }

    //----------------------------------------------------------------------------------//

    static int& private_event_set()
    {
        static thread_local int _instance = PAPI_NULL;
        return _instance;
    }

public:
    //----------------------------------------------------------------------------------//

    static void cleanup() {}
};

//--------------------------------------------------------------------------------------//
}  // namespace component
}  // namespace tim
