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
#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/data/Error_Correction.hpp"
#include "Fenrir/v1/db/Db.hpp"
#include "Fenrir/v1/util/Shared_Lock.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
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

class Loader;

template <typename T>
class FENRIR_LOCAL Plugin {
public:
    Plugin();
    Plugin (const Plugin&) = default;
    Plugin& operator= (const Plugin&) = default;
    Plugin (Plugin &&) = default;
    Plugin& operator= (Plugin &&) = default;
    ~Plugin() = default;

    std::shared_ptr<T> get_shared (Event::Loop *const loop,Loader *const loader,
                                    Random *const rnd, const typename T::ID id);
    bool add (const typename T::ID id, const std::shared_ptr<Lib> lib);
    std::vector<typename T::ID> list();
    bool del (const typename T::ID id);
    void del_all (const std::shared_ptr<Lib> lib);
    typename T::ID choose (const gsl::span<typename T::ID> supported);
    bool has (const typename T::ID id);
    bool set_preferences (std::vector<typename T::ID> &pref);
    std::vector<typename T::ID> get_preferences();
private:
    Shared_Lock lock;
    std::vector<std::pair<typename T::ID, std::shared_ptr<Lib>>> plugin;
    std::vector<typename T::ID> preferences;
};


class FENRIR_LOCAL Loader
{
public:
    Loader (Event::Loop *const loop, Random *const rnd);
    Loader () = delete;
    Loader (const Loader&) = delete;
    Loader& operator= (const Loader&) = delete;
    Loader (Loader &&) = delete;
    Loader& operator= (Loader &&) = delete;
    ~Loader() = default;

    Error load_directory (const std::string &dir);
    bool load_file (const std::string &file);
    bool del_file (const std::string &file);

    template<typename T>
    bool del_plugin (const typename T::ID id);
    template<typename T>
    std::vector<typename T::ID> list ();
    template<typename T>
    std::shared_ptr<T> get_shared (const typename T::ID id);
    template<typename T>
    typename T::ID choose (const gsl::span<typename T::ID> supported);
    template<typename T>
    bool has (const typename T::ID id);
    template<typename T>
    bool set_preferences (std::vector<typename T::ID> &pref);
    template<typename T>
    std::vector<typename T::ID> get_preferences();

private:
    Event::Loop *const _loop;
    Random *const _rnd;
    Shared_Lock lock;
    std::unordered_map<std::string, std::shared_ptr<Lib>> loaded;
    Plugin<Crypto::Encryption> _enc;
    Plugin<Crypto::Hmac> _hmac;
    Plugin<Crypto::Key> _key;
    Plugin<Crypto::KDF> _kdf;
    Plugin<Db> _db;
    Plugin<Crypto::Auth> _auth;
    Plugin<Rate::Rate> _rate;
    Plugin<Resolve::Resolver> _resolver;
    Plugin<Recover::ECC> _ecc;
};

} // namespace Impl
} // namespace Fenrir__v1

