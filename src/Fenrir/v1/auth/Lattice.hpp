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
#include <gsl/span>
#include <memory>
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {


class FENRIR_LOCAL Lattice_Node
{
public:
    std::vector<uint8_t> _name;
    std::vector<std::weak_ptr<Lattice_Node>> _parents;
    std::vector<std::shared_ptr<Lattice_Node>> _children;
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint8_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    ID _id;

    Lattice_Node() = delete;
    Lattice_Node (const ID id, const gsl::span<const uint8_t> name)
        :_id (id)
    {
        _name.reserve (static_cast<size_t> (name.size()));
        std::copy (name.begin(), name.end(), std::back_inserter (_name));
    }

    Lattice_Node (const Lattice_Node&) = default;
    Lattice_Node& operator= (const Lattice_Node&) = default;
    Lattice_Node (Lattice_Node &&) = default;
    Lattice_Node& operator= (Lattice_Node &&) = default;
    ~Lattice_Node() = default;
};

class FENRIR_LOCAL Lattice
{
public:
    static const constexpr Lattice_Node::ID TOP {0x00}, BOTTOM {0xFF};
    // id 0 is reserved (top), id 255 is reserved (bottom)
    Lattice ();
    Lattice (const gsl::span<const uint8_t> raw);
    Lattice (const Lattice&) = default;
    Lattice& operator= (const Lattice&) = default;
    Lattice (Lattice &&) = default;
    Lattice& operator= (Lattice &&) = default;
    ~Lattice() = default;

    operator bool() const;
    bool exists (const Lattice_Node::ID id) const;
    bool includes (const Lattice_Node::ID parent,
                                            const Lattice_Node::ID child) const;
    bool add_node (const Lattice_Node::ID id,
                                const gsl::span<const uint8_t> name,
                                const std::vector<Lattice_Node::ID> parents);
    size_t raw_size() const;
    // output format:
    // 1b  Node ID
    // 1b  Name length (max 30) + Xbytes: raw name
    // 1b  number of parents + list of parents (1b each)
    bool write (gsl::span<uint8_t> out) const;

private:
    static constexpr uint8_t MAX_NAME = 30;
    std::shared_ptr<Lattice_Node> _top;

    std::shared_ptr<Lattice_Node> find(const std::shared_ptr<Lattice_Node> from,
                                            const Lattice_Node::ID id) const;
};


} // namespace Impl
} // namespace Fenrir__v1
