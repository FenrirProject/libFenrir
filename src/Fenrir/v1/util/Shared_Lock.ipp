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

#include "Fenrir/v1/util/Shared_Lock.hpp"
#include <cassert>

namespace Fenrir__v1 {
namespace Impl {

template<>
FENRIR_INLINE Shared_Lock_Guard<Shared_Lock_Read>::Shared_Lock_Guard (
                                                            Shared_Lock_NN lock)
    :_lock (lock.modify().get())
    { _lock->lock_shared(); }

template<>
FENRIR_INLINE Shared_Lock_Guard<Shared_Lock_Write>::Shared_Lock_Guard (
                                                            Shared_Lock_NN lock)
    :_lock (lock.modify().get())
    { _lock->lock(); }

template<typename T>
FENRIR_INLINE Shared_Lock_Guard<T>::Shared_Lock_Guard (Shared_Lock_NN lock,
                                                            const bool unsafe)
    : _lock (lock.modify().get())
{
    FENRIR_UNUSED(unsafe);
    assert(unsafe);
}

template<>
FENRIR_INLINE Shared_Lock_Guard<Shared_Lock_Read>::~Shared_Lock_Guard()
{
    if (_lock != nullptr)
        _lock->unlock_shared();
}

template<>
FENRIR_INLINE Shared_Lock_Guard<Shared_Lock_Write>::~Shared_Lock_Guard()
{
    if (_lock != nullptr)
        _lock->unlock();
}

template<>
FENRIR_INLINE void Shared_Lock_Guard<Shared_Lock_Read>::early_unlock()
{
    assert(_lock != nullptr && "Shared_Lock_Read: nullptr");
    _lock->unlock_shared();
    _lock = nullptr;
}

template<>
FENRIR_INLINE void Shared_Lock_Guard<Shared_Lock_Write>::early_unlock()
{
    assert(_lock != nullptr && "Shared_Lock_Read: nullptr");
    _lock->unlock();
    _lock = nullptr;
}

template<>
template <typename T1>  // only for write
FENRIR_INLINE typename std::enable_if_t<
                                    std::is_same<T1, Shared_Lock_Write>::value,
                                            Shared_Lock_Guard<Shared_Lock_Read>>
                            Shared_Lock_Guard<Shared_Lock_Write>::downgrade()
{
    _lock->lock_unsafe();
    _lock->unlock();
    auto ret_ptr = Shared_Lock_NN {_lock};
    _lock = nullptr;
    return Shared_Lock_Guard<Shared_Lock_Read> (ret_ptr, true);
}

template<>
template <typename T1>  // only for read
FENRIR_INLINE typename std::enable_if_t<
                                    std::is_same<T1, Shared_Lock_Read>::value,
            std::pair<Shared_Lock_STAT, Shared_Lock_Guard<Shared_Lock_Write>>>
                            Shared_Lock_Guard<Shared_Lock_Read>::try_upgrade()
{
    auto save = Shared_Lock_NN {_lock};
    if (_lock->try_lock()) {
        _lock = nullptr;
        return {Shared_Lock_STAT::UPGRADED,
                            Shared_Lock_Guard<Shared_Lock_Write> (save, true)};
    }
    _lock->unlock_shared();
    _lock = nullptr;
    return {Shared_Lock_STAT::UNLOCKED_AND_RELOCKED,
                                Shared_Lock_Guard<Shared_Lock_Write> (save)};
}

} // namespace Impl
} // namespace Fenrir__v1
