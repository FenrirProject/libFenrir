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

#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/recover/Error_Correction.hpp"
#include "Fenrir/v1/util/Shared_Lock.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
#include "Fenrir/v1/plugin/Loader.hpp"
#include "Fenrir/v1/rate/Rate.hpp"
#include "Fenrir/v1/resolve/Resolver.hpp"
#include "Fenrir/v1/util/span_overlay.hpp"
#include <algorithm>
#include <cassert>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>


// The Fenrir::Lib class is used to load a shared library, implementation
// is os-depenent.
// This Loader searches all the plugins in the shared library, checks
// that the native algorithms are not be overridden, and gives us a nice
// template interface.
//
// As it is now a shared library is opened multiple times, one per each
// plugin type. It's not a big deal since it doesn't mean that the os loads it
// more than once. This setup should help a little with typesafety, and avoid
// a lot of up/downcasting.

namespace Fenrir__v1 {
namespace Impl {

/////////
// Plugin
/////////

template<typename T>
Plugin<T>::Plugin()
{
    static_assert(std::is_same<T, Crypto::Encryption>::value ||
                    std::is_same<T, Crypto::Hmac>::value ||
                    std::is_same<T, Crypto::Key>::value ||
                    std::is_same<T, Crypto::KDF>::value ||
                    std::is_same<T, Db>::value ||
                    std::is_same<T, Rate::Rate>::value ||
                    std::is_same<T, Resolve::Resolver>::value ||
                    std::is_same<T, Recover::ECC>::value ||
                    std::is_same<T, Crypto::Auth>::value,
                    "Fenrir Plugin type not supported");
    auto native = Lib::native();
    const auto list = native->list_supported<T>();
    plugin.reserve (list.size());

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto id : list)
        plugin.emplace_back (id, native);
    #pragma clang diagnostic pop
    // do NOT load previous loaded plugins, and do NOT override native ones

    uint16_t max_native = std::numeric_limits<uint16_t>::max();
    if (std::is_same<T, Crypto::Encryption>::value) {
        max_native = NATIVE_CIPHER;
    } else if (std::is_same<T, Crypto::Hmac>::value) {
        max_native = NATIVE_HMAC;
    } else if (std::is_same<T, Crypto::Key>::value) {
        max_native = NATIVE_KEY;
    } else if (std::is_same<T, Crypto::KDF>::value) {
        max_native = NATIVE_KDF;
    } else if (std::is_same<T, Db>::value) {
        max_native = NATIVE_DB;
    } else if (std::is_same<T, Rate::Rate>::value) {
        max_native = NATIVE_AUTH;
    } else if (std::is_same<T, Resolve::Resolver>::value) {
        max_native = NATIVE_RATE;
    } else if (std::is_same<T, Recover::ECC>::value) {
        max_native = NATIVE_ECC;
    } else if (std::is_same<T, Crypto::Auth>::value) {
        max_native = NATIVE_RESOLV;
    }

    // put preferences in backwards order.
    // rationale: high id: developed/tested afterwards, therefore "better"
    for (uint16_t idx = max_native; idx > 0; --idx)
        preferences.push_back (typename T::ID {idx});
}

template <typename T>
bool Plugin<T>::del (const typename T::ID id)
{
    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    for (auto it = preferences.begin(); it != preferences.end(); it++) {
        if (*it == id) {
            preferences.erase(it);
            break;
        }
    }

    auto it = std::lower_bound (plugin.begin(), plugin.end(), id,
                    [] (const auto &test, const typename T::ID idx)
                        { return std::get<typename T::ID> (test) < idx; });
    if (it == plugin.end() || std::get<typename T::ID> (*it) != id)
        return false;
    plugin.erase (it);

    return true;
}

template<typename T>
void Plugin<T>::del_all (const std::shared_ptr<Lib> lib)
{
    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    for (auto it = plugin.begin(); it != plugin.end(); it++) {
        if (it->second == lib) {
            for (auto it_pref = preferences.begin();
                                    it_pref != preferences.end(); ++it_pref) {
                if (*it_pref == it->first) {
                    preferences.erase(it_pref);
                    break;
                }
            }
            plugin.erase(it);
        }
    }
}

template<typename T>
bool Plugin<T>::add (const typename T::ID id, const std::shared_ptr<Lib> lib)
{
    // do NOT load previous loaded plugins, and do NOT override native ones

    uint16_t max_native = std::numeric_limits<uint16_t>::max();
    if (std::is_same<T, Crypto::Encryption>::value) {
        max_native = NATIVE_CIPHER;
    } else if (std::is_same<T, Crypto::Hmac>::value) {
        max_native = NATIVE_HMAC;
    } else if (std::is_same<T, Crypto::Key>::value) {
        max_native = NATIVE_KEY;
    } else if (std::is_same<T, Crypto::KDF>::value) {
        max_native = NATIVE_KDF;
    } else if (std::is_same<T, Db>::value) {
        max_native = NATIVE_DB;
    } else if (std::is_same<T, Rate::Rate>::value) {
        max_native = NATIVE_AUTH;
    } else if (std::is_same<T, Resolve::Resolver>::value) {
        max_native = NATIVE_RATE;
    } else if (std::is_same<T, Recover::ECC>::value) {
        max_native = NATIVE_ECC;
    } else if (std::is_same<T, Crypto::Auth>::value) {
        max_native = NATIVE_RESOLV;
    }
    if (static_cast<uint16_t> (id) <= max_native)
        return false;

    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    auto it_plg = std::lower_bound (plugin.begin(), plugin.end(), id,
                    [] (const auto &test, const typename T::ID idx)
                        { return std::get<typename T::ID> (test) < idx; });

    if (it_plg != plugin.end() && std::get<typename T::ID> (*it_plg) == id)
        return false;
    plugin.emplace (it_plg, id, lib);
    preferences.push_back (id);
    return true;
}

template<typename T>
std::vector<typename T::ID> Plugin<T>::list()
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(rlock);

    std::vector<typename T::ID> ret;
    ret.reserve (plugin.size());
    for (auto it = plugin.begin(); it != plugin.end(); it++)
        ret.emplace_back (it->first);

    return ret;
}

template<typename T>
std::shared_ptr<T> Plugin<T>::get_shared (
                                Event::Loop *const loop, Loader *const loader,
                                    Random *const rnd, const typename T::ID id)
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(rlock);

    auto it = std::lower_bound (plugin.begin(), plugin.end(), id,
                    [] (const auto &test, const typename T::ID idx)
                        { return std::get<typename T::ID> (test) < idx; });

    if (it != plugin.end() && std::get<typename T::ID> (*it) == id) {
        auto lib = std::get<std::shared_ptr<Lib>> (*it);
        return lib->template get_plugin<T> (loop, loader, rnd,
                                    std::get<std::shared_ptr<Lib>> (*it), id);
    }
    return nullptr;
}

template<typename T>
bool Plugin<T>::set_preferences (std::vector<typename T::ID> &pref)
{
    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    // check for bogous list:
    if (pref.size() != preferences.size())
        return false;
    std::vector<bool> test;
    test.reserve (preferences.size());
    std::fill (test.begin(), test.end(), false);
    // check for duplicates
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto id : pref) {
        if (test[static_cast<uint16_t>(id)])
            return false;
        test[static_cast<uint16_t> (id)] = true;
    }
    #pragma clang diagnostic pop
    preferences = std::move(pref);
    return true;
}

template<typename T>
std::vector<typename T::ID> Plugin<T>::get_preferences()
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(rlock);
    return preferences;
}

template<typename T>
typename T::ID Plugin<T>::choose (const gsl::span<typename T::ID> supported)
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(rlock);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto srv_id : preferences) {
        for (const auto client_id : supported) {
            if (srv_id == client_id)
                return srv_id;
        }
    }
    #pragma clang diagnostic pop
    return typename T::ID {0};
}

template<typename T>
bool Plugin<T>::has (const typename T::ID id)
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(rlock);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const typename T::ID test : preferences) {
        if (id == test)
            return true;
    }
    #pragma clang diagnostic pop
    return false;
}

/////////
// Loader
/////////

Loader::Loader (Event::Loop *const loop, Random *const rnd)
    : _loop (loop), _rnd (rnd)
{}


bool Loader::load_file (const std::string &file)
{
    // Load plugins from the shared library.

    std::shared_ptr<Lib> lib;

    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    if (loaded.find(file) == loaded.end()) {
        // new file : load it
        lib = std::make_shared<Lib> (file);
        if (!(*lib))
            return false;
        // ok, now it's safe to load.
        loaded[file] = lib;
    } else {
        lib = loaded[file]; // reuse, no sense in reloading
    }

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto id : lib->list_supported<Crypto::Encryption>())
        _enc.add (id, lib);
    for (const auto id : lib->list_supported<Crypto::Hmac>())
        _hmac.add (id, lib);
    for (const auto id : lib->list_supported<Crypto::Key>())
        _key.add (id, lib);
    for (const auto id : lib->list_supported<Crypto::KDF>())
        _kdf.add (id, lib);
    for (const auto id : lib->list_supported<Db>())
        _db.add (id, lib);
    for (const auto id : lib->list_supported<Rate::Rate>())
        _rate.add (id, lib);
    for (const auto id : lib->list_supported<Resolve::Resolver>())
        _resolver.add (id, lib);
    for (const auto id : lib->list_supported<Crypto::Auth>())
        _auth.add (id, lib);
    for (const auto id : lib->list_supported<Recover::ECC>())
        _ecc.add (id, lib);
    #pragma clang diagnostic pop

    return true;
}

bool Loader::del_file (const std::string &file)
{
    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN {&lock});
    FENRIR_UNUSED(wlock);

    auto it = loaded.find (file);

    if (it == loaded.end())
        return false;

    _enc.del_all (it->second);
    _hmac.del_all (it->second);
    _key.del_all (it->second);
    _kdf.del_all (it->second);
    _db.del_all (it->second);
    _rate.del_all (it->second);
    _resolver.del_all (it->second);
    _auth.del_all (it->second);
    _ecc.del_all (it->second);

    loaded.erase (it);

    return true;
}

template<>
bool Loader::del_plugin<Crypto::Encryption> (
                                    const typename Crypto::Encryption::ID id)
    { return _enc.del (id); }
template<>
bool Loader::del_plugin<Crypto::Hmac> (
                                    const typename Crypto::Hmac::ID id)
    { return _hmac.del (id); }
template<>
bool Loader::del_plugin<Crypto::Key> (
                                    const typename Crypto::Key::ID id)
    { return _key.del (id); }
template<>
bool Loader::del_plugin<Crypto::KDF> (
                                    const typename Crypto::KDF::ID id)
    { return _kdf.del (id); }
template<>
bool Loader::del_plugin<Db> (
                                    const typename Db::ID id)
    { return _db.del (id); }
template<>
bool Loader::del_plugin<Rate::Rate> (
                                    const typename Rate::Rate::ID id)
    { return _rate.del (id); }
template<>
bool Loader::del_plugin<Resolve::Resolver> (
                                    const typename Resolve::Resolver::ID id)
    { return _resolver.del (id); }
template<>
bool Loader::del_plugin<Recover::ECC> (
                                    const typename Recover::ECC::ID id)
    { return _ecc.del (id); }
template<>
bool Loader::del_plugin<Crypto::Auth> (
                                    const typename Crypto::Auth::ID id)
    { return _auth.del (id); }

template<>
std::vector<Crypto::Encryption::ID> Loader::list<Crypto::Encryption>()
    { return _enc.list(); }
template<>
std::vector<Crypto::Hmac::ID> Loader::list<Crypto::Hmac>()
    { return _hmac.list(); }
template<>
std::vector<Crypto::Key::ID> Loader::list<Crypto::Key>()
    { return _key.list(); }
template<>
std::vector<Crypto::KDF::ID> Loader::list<Crypto::KDF>()
    { return _kdf.list(); }
template<>
std::vector<Db::ID> Loader::list<Db>()
    { return _db.list(); }
template<>
std::vector<Rate::Rate::ID> Loader::list<Rate::Rate>()
    { return _rate.list(); }
template<>
std::vector<Resolve::Resolver::ID> Loader::list<Resolve::Resolver>()
    { return _resolver.list(); }
template<>
std::vector<Recover::ECC::ID> Loader::list<Recover::ECC>()
    { return _ecc.list(); }
template<>
std::vector<Crypto::Auth::ID> Loader::list<Crypto::Auth> ()
    { return _auth.list(); }

template<>
std::shared_ptr<Crypto::Encryption> Loader::get_shared<Crypto::Encryption> (
                                    const typename Crypto::Encryption::ID id)
    { return _enc.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Crypto::Hmac> Loader::get_shared<Crypto::Hmac> (
                                    const typename Crypto::Hmac::ID id)
    { return _hmac.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Crypto::Key> Loader::get_shared<Crypto::Key> (
                                    const typename Crypto::Key::ID id)
    { return _key.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Crypto::KDF> Loader::get_shared<Crypto::KDF> (
                                    const typename Crypto::KDF::ID id)
    { return _kdf.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Db> Loader::get_shared<Db> (
                                    const typename Db::ID id)
    { return _db.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Rate::Rate> Loader::get_shared<Rate::Rate> (
                                    const typename Rate::Rate::ID id)
    { return _rate.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Resolve::Resolver> Loader::get_shared<Resolve::Resolver> (
                                    const typename Resolve::Resolver::ID id)
    { return _resolver.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Recover::ECC> Loader::get_shared<Recover::ECC> (
                                    const typename Recover::ECC::ID id)
    { return _ecc.get_shared (_loop, this, _rnd, id); }
template<>
std::shared_ptr<Crypto::Auth> Loader::get_shared<Crypto::Auth> (
                                    const typename Crypto::Auth::ID id)
    { return _auth.get_shared (_loop, this, _rnd, id); }

template<>
bool Loader::set_preferences<Crypto::Encryption> (
                                    std::vector<Crypto::Encryption::ID> &pref)
    { return _enc.set_preferences (pref); }
template<>
bool Loader::set_preferences<Crypto::Hmac> (
                                    std::vector<Crypto::Hmac::ID> &pref)
    { return _hmac.set_preferences (pref); }
template<>
bool Loader::set_preferences<Crypto::Key> (
                                    std::vector<Crypto::Key::ID> &pref)
    { return _key.set_preferences (pref); }
template<>
bool Loader::set_preferences<Crypto::KDF> (
                                    std::vector<Crypto::KDF::ID> &pref)
    { return _kdf.set_preferences (pref); }
template<>
bool Loader::set_preferences<Db> (
                                    std::vector<Db::ID> &pref)
    { return _db.set_preferences (pref); }
template<>
bool Loader::set_preferences<Rate::Rate> (
                                    std::vector<Rate::Rate::ID> &pref)
    { return _rate.set_preferences (pref); }
template<>
bool Loader::set_preferences<Resolve::Resolver> (
                                    std::vector<Resolve::Resolver::ID> &pref)
    { return _resolver.set_preferences (pref); }
template<>
bool Loader::set_preferences<Recover::ECC> (
                                    std::vector<Recover::ECC::ID> &pref)
    { return _ecc.set_preferences (pref); }
template<>
bool Loader::set_preferences<Crypto::Auth> (
                                    std::vector<Crypto::Auth::ID> &pref)
    { return _auth.set_preferences (pref); }


template<>
std::vector<Crypto::Encryption::ID> Loader::get_preferences
                                                        <Crypto::Encryption>()
    { return _enc.list(); }
template<>
std::vector<Crypto::Hmac::ID> Loader::get_preferences<Crypto::Hmac>()
    { return _hmac.list(); }
template<>
std::vector<Crypto::Key::ID> Loader::get_preferences<Crypto::Key>()
    { return _key.list(); }
template<>
std::vector<Crypto::KDF::ID> Loader::get_preferences<Crypto::KDF>()
    { return _kdf.list(); }
template<>
std::vector<Db::ID> Loader::get_preferences<Db>()
    { return _db.list(); }
template<>
std::vector<Rate::Rate::ID> Loader::get_preferences<Rate::Rate>()
    { return _rate.list(); }
template<>
std::vector<Resolve::Resolver::ID> Loader::get_preferences<Resolve::Resolver>()
    { return _resolver.list(); }
template<>
std::vector<Recover::ECC::ID> Loader::get_preferences<Recover::ECC>()
    { return _ecc.list(); }
template<>
std::vector<Crypto::Auth::ID> Loader::get_preferences<Crypto::Auth> ()
    { return _auth.list(); }


template<>
Crypto::Encryption::ID Loader::choose<Crypto::Encryption> (
                                gsl::span<Crypto::Encryption::ID> supported)
    { return _enc.choose (supported); }
template<>
Crypto::Hmac::ID Loader::choose<Crypto::Hmac> (
                                gsl::span<Crypto::Hmac::ID> supported)
    { return _hmac.choose (supported); }
template<>
Crypto::Key::ID Loader::choose<Crypto::Key> (
                                gsl::span<Crypto::Key::ID> supported)
    { return _key.choose (supported); }
template<>
Crypto::KDF::ID Loader::choose<Crypto::KDF> (
                                gsl::span<Crypto::KDF::ID> supported)
    { return _kdf.choose (supported); }
template<>
Db::ID Loader::choose<Db> (     gsl::span<Db::ID> supported)
    { return _db.choose (supported); }
template<>
Rate::Rate::ID Loader::choose<Rate::Rate> (
                                gsl::span<Rate::Rate::ID> supported)
    { return _rate.choose (supported); }
template<>
Resolve::Resolver::ID Loader::choose<Resolve::Resolver> (
                                gsl::span<Resolve::Resolver::ID> supported)
    { return _resolver.choose (supported); }
template<>
Recover::ECC::ID Loader::choose<Recover::ECC> (
                                gsl::span<Recover::ECC::ID> supported)
    { return _ecc.choose (supported); }
template<>
Crypto::Auth::ID Loader::choose<Crypto::Auth> (
                                gsl::span<Crypto::Auth::ID> supported)
    { return _auth.choose (supported); }


template<>
bool Loader::has<Crypto::Encryption> (
                                    const typename Crypto::Encryption::ID id)
    { return _enc.has (id); }
template<>
bool Loader::has<Crypto::Hmac> (
                                    const typename Crypto::Hmac::ID id)
    { return _hmac.has (id); }
template<>
bool Loader::has<Crypto::Key> (
                                    const typename Crypto::Key::ID id)
    { return _key.has (id); }
template<>
bool Loader::has<Crypto::KDF> (
                                    const typename Crypto::KDF::ID id)
    { return _kdf.has (id); }
template<>
bool Loader::has<Db> (
                                    const typename Db::ID id)
    { return _db.has (id); }
template<>
bool Loader::has<Rate::Rate> (
                                    const typename Rate::Rate::ID id)
    { return _rate.has (id); }
template<>
bool Loader::has<Resolve::Resolver> (
                                    const typename Resolve::Resolver::ID id)
    { return _resolver.has (id); }
template<>
bool Loader::has<Recover::ECC> (
                                    const typename Recover::ECC::ID id)
    { return _ecc.has (id); }
template<>
bool Loader::has<Crypto::Auth> (
                                    const typename Crypto::Auth::ID id)
    { return _auth.has (id); }

} // namespace Impl
} // namespace Fenrir__v1
