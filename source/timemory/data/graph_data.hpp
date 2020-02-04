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

#pragma once

//--------------------------------------------------------------------------------------//

#include "timemory/data/graph.hpp"
#include "timemory/settings.hpp"

#include <cstdint>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>

//--------------------------------------------------------------------------------------//
//
namespace tim
{
//--------------------------------------------------------------------------------------//
//
//  graph instance + current node + head node
//
//--------------------------------------------------------------------------------------//

template <typename _Node>
class graph_data
{
public:
    using this_type          = graph_data<_Node>;
    using graph_t            = tim::graph<_Node>;
    using iterator           = typename graph_t::iterator;
    using const_iterator     = typename graph_t::const_iterator;
    using inverse_insert_t   = std::vector<std::pair<int64_t, iterator>>;
    using pre_order_iterator = typename graph_t::pre_order_iterator;
    using sibling_iterator   = typename graph_t::sibling_iterator;

public:
    // graph_data() = default;

    explicit graph_data(const _Node& rhs, int64_t _depth, graph_data* _master = nullptr)
    : m_has_head(true)
    , m_depth(_depth)
    , m_sea_level(_depth)
    , m_master(_master)
    {
        m_head    = m_graph.set_head(rhs);
        m_current = m_head;
        m_dummies.insert({ m_depth, m_current });
    }

    ~graph_data() { m_graph.clear(); }

    // allow move and copy construct
    graph_data(this_type&&) = delete;
    graph_data& operator=(this_type&&) = delete;

    // delete copy-assignment
    graph_data(const this_type&) = delete;
    graph_data& operator=(const this_type&) = delete;

    bool has_head() const { return m_has_head; }

    const int64_t& depth() const { return m_depth; }
    const graph_t& graph() const { return m_graph; }
    const int64_t& sea_level() const { return m_sea_level; }

    int64_t&  depth() { return m_depth; }
    graph_t&  graph() { return m_graph; }
    iterator& current() { return m_current; }
    iterator& head() { return m_head; }
    int64_t&  sea_level() { return m_sea_level; }

    iterator       begin() { return m_graph.begin(); }
    iterator       end() { return m_graph.end(); }
    const_iterator begin() const { return m_graph.begin(); }
    const_iterator end() const { return m_graph.end(); }

    inline void clear()
    {
        m_graph.clear();
        m_has_head  = false;
        m_depth     = 0;
        m_sea_level = 0;
        m_current   = nullptr;
        m_dummies.clear();
    }

    inline void set_master(graph_data* _master)
    {
        if(_master != this)
            m_master = _master;
    }

    inline void add_dummy()
    {
        if(!m_master)
            return;

        auto _current = m_master->current();
        // auto _parent  = graph_t::parent(m_current);
        // if(_parent)
        //    _current = _parent;

        auto _id    = _current->id();
        auto _depth = _current->depth();

        _Node node(_id, _Node::get_dummy(), _depth);
        m_depth     = _depth;
        m_sea_level = _depth;

        m_current = m_graph.insert_after(m_head, node);
        // m_current = m_graph.append_child(m_head, node);

        m_dummies.insert({ m_depth, m_current });
    }

    inline void reset()
    {
        m_graph.erase_children(m_head);
        m_depth   = 0;
        m_current = m_head;
    }

    inline iterator pop_graph()
    {
        if(m_depth > 0 && !m_graph.is_head(m_current))
        {
            --m_depth;
            m_current = graph_t::parent(m_current);
        }
        else if(m_depth == 0)
        {
            m_current = m_head;
        }
        return m_current;
    }

    inline iterator find(iterator itr)
    {
        if(!itr)
            return end();

        for(pre_order_iterator fitr = begin(); fitr != end(); ++fitr)
        {
            if(fitr && *itr == *fitr && get_rolling_hash(itr) == get_rolling_hash(fitr))
                return fitr;
            sibling_iterator entry = fitr;
            for(auto sitr = entry.begin(); sitr != entry.end(); ++sitr)
            {
                if(sitr && *itr == *sitr &&
                   get_rolling_hash(itr) == get_rolling_hash(sitr))
                    return sitr;
            }
        }
        return end();
    }

    inline int64_t get_rolling_hash(iterator itr)
    {
        if(itr)
            return 0;
        auto _parent         = graph_t::parent(itr);
        using hierarchy_type = std::vector<int64_t>;
        auto _hierarchy      = hierarchy_type{};
        while(_parent)
        {
            _hierarchy.push_back(_parent->id());
            _parent = graph_t::parent(_parent);
            if(!_parent)
                break;
        }
        if(_hierarchy.size() > 1)
            std::reverse(_hierarchy.begin(), _hierarchy.end());
        _hierarchy.push_back(itr->id());
        return std::accumulate(_hierarchy.begin(), _hierarchy.end(), 0);
    }

    inline iterator append_child(_Node& node)
    {
        ++m_depth;
        return (m_current = m_graph.append_child(m_current, node));
    }

    inline iterator append_head(_Node& node)
    {
        return m_graph.append_child(m_head, node);
    }

    inline iterator emplace_child(iterator _itr, _Node& node)
    {
        return m_graph.append_child(_itr, node);
    }

    bool is_dummy(iterator itr) const { return (m_dummies.find(itr) != m_dummies.end()); }

    bool at_sea_level() const { return (m_depth == m_sea_level); }

    inverse_insert_t get_inverse_insert() const
    {
        inverse_insert_t ret;
        for(const auto& itr : m_dummies)
            ret.push_back({ itr.first, itr.second });
        std::reverse(ret.begin(), ret.end());
        return ret;
    }

private:
    bool                             m_has_head  = false;
    int64_t                          m_depth     = 0;
    int64_t                          m_sea_level = 0;
    graph_t                          m_graph;
    iterator                         m_current = nullptr;
    iterator                         m_head    = nullptr;
    graph_data*                      m_master  = nullptr;
    std::multimap<int64_t, iterator> m_dummies = {};
};
}  // namespace tim