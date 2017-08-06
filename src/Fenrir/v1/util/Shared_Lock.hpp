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

#include "Fenrir/v1/common.hpp"
#include <atomic>
#include <mutex>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include <type_safe/constrained_type.hpp>
#pragma clang diagnostic pop
#include <type_traits>
#include <utility>

namespace Fenrir__v1 {
namespace Impl {

class Shared_Lock;
class Shared_Lock_Read;
class Shared_Lock_Write;
using Shared_Lock_NN = typename type_safe::constrained_type<Shared_Lock*,
                                            type_safe::constraints::non_null>;
enum class Shared_Lock_STAT : uint8_t {
                                UPGRADED = 0x01, UNLOCKED_AND_RELOCKED = 0x02 };

template<typename T>
class FENRIR_LOCAL Shared_Lock_Guard
{
public:
    Shared_Lock_Guard (Shared_Lock_NN _lock);

    Shared_Lock_Guard () = delete;
    Shared_Lock_Guard (const Shared_Lock_Guard&) = delete;
    Shared_Lock_Guard& operator= (const Shared_Lock_Guard&) = delete;
    Shared_Lock_Guard (Shared_Lock_Guard&&) = default;
    Shared_Lock_Guard& operator= (Shared_Lock_Guard&&) = default;
    ~Shared_Lock_Guard();

    void early_unlock();

    // try to upgrade the lock.
    // if an other thread is waiting, instead of upgrading
    // do an unlock-relock to avoid deadlock when 2 threads are upgrading
    template <typename T1 = T> // only for read
    typename std::enable_if_t<std::is_same<T1, Shared_Lock_Read>::value,
            std::pair<Shared_Lock_STAT, Shared_Lock_Guard<Shared_Lock_Write>>>
                                                                try_upgrade();

    template <typename T1 = T> // only for write
    typename std::enable_if_t<std::is_same<T1, Shared_Lock_Write>::value,
            Shared_Lock_Guard<Shared_Lock_Read>> downgrade ();
private:
    Shared_Lock *_lock;

    // deferred locking for upgrades and downgrades
    friend class Shared_Lock_Guard<Shared_Lock_Read>;
    friend class Shared_Lock_Guard<Shared_Lock_Write>;
    Shared_Lock_Guard (Shared_Lock_NN lock, const bool unsafe);
};

// this lock should be used only with lock_guard, as it is kinda easy
// to mess up
class FENRIR_LOCAL Shared_Lock
{
public:
    Shared_Lock()
        : _readers (0) {}
    Shared_Lock (const Shared_Lock&) = delete;
    Shared_Lock& operator= (const Shared_Lock&) = delete;
    Shared_Lock (Shared_Lock &&rhs)
    {
        std::lock_guard<std::mutex> (rhs._mtx);
        _readers.exchange (rhs._readers);
    }

    Shared_Lock& operator= (Shared_Lock &&rhs)
    {
        std::lock_guard<std::mutex> (rhs._mtx);
        _readers.exchange (rhs._readers);
        return *this;
    }
    ~Shared_Lock() {}

private:
    void lock()
    {
        _mtx.lock();
        while (_readers.load() != 0){
            ;	// busy wait. Not really good. Better ideas?
        }
    }
    bool try_lock() // used ONLY in upgrading
    {
        if (!_mtx.try_lock())
            return false;
        unlock_shared();
        while (_readers.load() != 0){
            ;	// busy wait. Not really good. Better ideas?
        }
        return true;
    }
    void unlock()
        { _mtx.unlock(); }
    void lock_shared ()
    {
        _mtx.lock();
        ++_readers;
        _mtx.unlock();
    }
    void unlock_shared()
        { --_readers; }
    void lock_unsafe() // use ONLY in Shared_Lock_Guard, for downgrade()
        { ++_readers; }
    std::mutex _mtx;
    std::atomic<uint32_t> _readers;
    friend class Shared_Lock_Guard<Shared_Lock_Read>;
    friend class Shared_Lock_Guard<Shared_Lock_Write>;
};

} // namespace Impl
} // namespace Fenrir__v1

#ifdef FENRIR_HEADER_ONLY
#include "Fenrir/v1/util/Shared_Lock.hpp"
#endif
