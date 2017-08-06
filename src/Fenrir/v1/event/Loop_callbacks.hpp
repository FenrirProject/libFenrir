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

#include <ev.h>

namespace Fenrir__v1 {
namespace Impl {
namespace Event {

void cb_io (struct ev_loop *loop, ev_io *ev, int ev_type);
void cb_timer (struct ev_loop *loop, ev_timer *ev, int ev_type);
void cb_running_hack (struct ev_loop *loop, ev_timer *ev, int ev_type);

} // namespace Event
} // namespace Impl
} // namespace Fenrir__v1

