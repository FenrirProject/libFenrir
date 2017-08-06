/*
 * Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#include <vector>
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/data/AS_list.hpp"
#include "Fenrir/v1/plugin/Dynamic.hpp"

namespace Fenrir__v1 {
namespace Impl {
class Loop;
class Lib;
class Loader;
namespace Event {
class Connect;
class Resolve;
} // namespace Event


namespace Resolve {

class FENRIR_LOCAL Resolver : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                        type_safe::strong_typedef<ID, uint16_t>,
                        type_safe::strong_typedef_op::equality_comparison<ID>,
                        type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };

    explicit Resolver (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}

    Resolver () = delete;
    Resolver (const Resolver&) = delete;
    Resolver& operator=(const Resolver&) = delete;
    Resolver (Resolver&&) = delete;
    Resolver& operator= (Resolver&&) = delete;
    virtual ~Resolver() {}

    virtual ID id() const = 0;

    virtual bool initialize() = 0;
    virtual void stop() = 0;
    virtual Impl::Error resolv_async (
                                std::shared_ptr<Event::Resolve> request) = 0;
};

} // namespace Resolve
} // namespace Impl
} // namespace Fenrir__v1
