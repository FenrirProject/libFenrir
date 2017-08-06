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

#define FENRIR_HEADER_ONLY
#include "Fenrir/v1/common.hpp"

#include "Fenrir/v1/util/Shared_Lock.ipp"
#include "Fenrir/v1/plugin/Loader.ipp"

#include "Fenrir/v1/data/control/Control.ipp"
#include "Fenrir/v1/data/Conn0.ipp"
#include "Fenrir/v1/data/packet/Stream.ipp"
#include "Fenrir/v1/data/Storage_t.ipp"
#include "Fenrir/v1/data/Storage_Raw.ipp"
#include "Fenrir/v1/event/Loop.ipp"
#include "Fenrir/v1/event/Loop_callbacks.ipp"
#include "Fenrir/v1/Handler.ipp"
#include "Fenrir/v1/net/Connection_Control.ipp"
#include "Fenrir/v1/net/Connection.ipp"
#include "Fenrir/v1/net/Link.ipp"
#include "Fenrir/v1/net/Handshake.ipp"
#include "Fenrir/v1/rate/Rate.ipp"

#include "Fenrir/Fenrir_v1.hpp"


