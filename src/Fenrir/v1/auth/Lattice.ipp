/*
 * Copyright (c) 2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of libFenrir.
 *
 * libFenrir is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libFenrir is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libFenrir.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/auth/Lattice.hpp"
#include <array>
#include <deque>

namespace Fenrir__v1 {
namespace Impl {

namespace {
constexpr std::array<const uint8_t, 3> lattice_a_TOP {{ 't', 'o', 'p'}};
constexpr std::array<const uint8_t, 6> lattice_a_BOTTOM {{
                                                    'b','o','t','t','o','m' }};
}

constexpr Lattice_Node::ID Lattice::TOP, Lattice::BOTTOM;

FENRIR_INLINE Lattice::Lattice()
{
    auto bottom = std::make_shared<Lattice_Node> (BOTTOM,
                                gsl::span<const uint8_t> {lattice_a_BOTTOM});
    _top = std::make_shared<Lattice_Node> (TOP,
                                    gsl::span<const uint8_t> {lattice_a_TOP});
    _top->_children.emplace_back (bottom);
    bottom->_parents.emplace_back (_top);
}

FENRIR_INLINE Lattice::Lattice (const gsl::span<const uint8_t> raw)
{
    auto bottom = std::make_shared<Lattice_Node> (BOTTOM,
                                gsl::span<const uint8_t> {lattice_a_BOTTOM});
    _top = std::make_shared<Lattice_Node> (TOP,
                                    gsl::span<const uint8_t> {lattice_a_TOP});
    _top->_children.emplace_back (bottom);
    bottom->_parents.emplace_back (_top);

    ssize_t idx = 0;
    while (idx < raw.size()) {
        if (raw.size() - idx < 4) {
            _top = nullptr;
            return;
        }
        const Lattice_Node::ID node_id {raw[idx++]};
        if (node_id == TOP || node_id == BOTTOM) {
            _top = nullptr;
            return;
        }
        const uint8_t name_len = raw[idx++];
        if (name_len > MAX_NAME || (idx + MAX_NAME) > raw.size()) {
            _top = nullptr;
            return;
        }
        const gsl::span<const uint8_t> name (raw.data() + idx, name_len);
        idx += name_len;
        const uint8_t parents_len = raw[idx++];
        if (idx + parents_len > raw.size()) {
            _top = nullptr;
            return;
        }
        std::vector<Lattice_Node::ID> parents (parents_len);
        while (parents_len > 0) {
            const Lattice_Node::ID parent_id {raw[idx]};
            if (parent_id == BOTTOM) {
                _top = nullptr;
                return;
            }
            parents.push_back (parent_id);
            ++idx;
        }
        if (!add_node (node_id, name, parents)) {
            _top = nullptr;
            return;
        }
    }
}

FENRIR_INLINE Lattice::operator bool() const
    { return _top != nullptr; }

FENRIR_INLINE bool Lattice::exists (const Lattice_Node::ID id) const
{
    if (find (_top, id) == nullptr)
        return false;
    return true;
}

FENRIR_INLINE bool Lattice::includes (const Lattice_Node::ID parent,
                                            const Lattice_Node::ID child) const
{
    auto node = find (_top, parent);
    if (find (node, child) == nullptr)
        return false;
    return true;
}

FENRIR_INLINE bool Lattice::add_node (const Lattice_Node::ID id,
                                    const gsl::span<const uint8_t> name,
                                    const std::vector<Lattice_Node::ID> parents)
{
    // limit name to 30 chars.
    if (name.size() > MAX_NAME)
        return false;
    if (find (_top, id) != nullptr)
        return false; // already present
    std::vector<std::shared_ptr<Lattice_Node>> sh_parents;
    sh_parents.reserve (parents.size());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto p : parents) {
#pragma clang diagnostic pop
        if (p == BOTTOM)
            return false;
        auto tmp =  find (_top, p);
        if (tmp == nullptr)
            return false; // parent missing
        sh_parents.push_back (tmp);
    }
    // find bottom quickly (find() is actually BFS)
    auto bot = _top;
    while (bot->_id != BOTTOM)
        bot = bot->_children.front();

    auto node = std::make_shared<Lattice_Node> (id, name);
    node->_children.push_back(find (_top, BOTTOM));
    for (auto p : sh_parents) {
        if (p->_children[0]->_id == BOTTOM)
            p->_children.clear();
        p->_children.push_back (node);
        node->_parents.push_back (p);
    }
    return true;
}

FENRIR_INLINE size_t Lattice::raw_size() const
{
    // BFS
    size_t ret_size = 0;
    std::deque<std::shared_ptr<Lattice_Node>> queue;
    // skip top and bottom, they do not need to be saved
    for (auto x : _top->_children) {
        if (x->_id == BOTTOM)
            break;
        queue.push_back (x);
    }
    while (queue.size() > 0) {
        auto x = queue.front();
        queue.pop_front();
        ret_size += 1 + // Node ID
                    1 + x->_name.size() + // length + name
                    1 + x->_parents.size(); // length + ids
        for (auto child : x->_children)
            queue.push_back (child);
    }
    return ret_size;
}

FENRIR_INLINE bool Lattice::write (gsl::span<uint8_t> out) const
{
    if (static_cast<size_t> (out.size()) != raw_size())
        return false;
    // BFS
    std::deque<std::shared_ptr<Lattice_Node>> queue;
    queue.push_back (_top);
    auto it = out.begin();
    while (queue.size() > 0) {
        auto x = queue.front();
        queue.pop_front();
        for (auto child : x->_children)
            queue.push_back (child);
        *it = static_cast<uint8_t> (x->_id);
        *it = static_cast<uint8_t> (x->_name.size());
        for (const uint8_t el : x->_name)
            *(it++) = el;
        *it = static_cast<uint8_t> (x->_parents.size());
        for (auto weak_p : x->_parents) {
            auto sh_p = weak_p.lock();
            assert (sh_p != nullptr && "Lattice_Write:: parent nullptr");
            *(it++) = static_cast<uint8_t> (sh_p->_id);
        }
    }
    return true;
}

FENRIR_INLINE std::shared_ptr<Lattice_Node> Lattice::find (
                                    const std::shared_ptr<Lattice_Node> from,
                                                const Lattice_Node::ID id) const
{
    // BFS
    std::deque<std::shared_ptr<Lattice_Node>> queue;
    queue.push_back (from);
    while (queue.size() > 0) {
        auto x = queue.front();
        queue.pop_front();
        if (x->_id == id)
            return x;
        for (auto child : x->_children)
            queue.push_back (child);
    }
    return nullptr;
}

} // namespace Impl
} // namespace Fenrir__v1
