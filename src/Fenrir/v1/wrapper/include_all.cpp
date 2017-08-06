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

#include "Fenrir/Fenrir_v1_hdr.hpp"

bool FENRIR_API lol_wut();
bool lol_wut()
{


    Fenrir__v1::Impl::Event::Loop loop;
    Fenrir__v1::Impl::Loader load (nullptr, nullptr);
    Fenrir__v1::Impl::Handler h;

    Fenrir__v1::Impl::Shared_Lock lk;
    Fenrir__v1::Impl::Shared_Lock_NN lk_nn(&lk);
    Fenrir__v1::Impl::Shared_Lock_Guard<Fenrir__v1::Impl::Shared_Lock_Write> test (lk_nn);
    auto read_lk = test.downgrade();

    return false;
}
