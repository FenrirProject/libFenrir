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
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include <dlfcn.h>
#include <memory>
#include <string>
#include <vector>

// See Loader.hpp for explanation
// This class is used to load all the plugins for a file.
// it's just RAII for the dlopen/dlsym syscalls

namespace Fenrir__v1 {
namespace Impl {

class Random;

constexpr uint32_t NATIVE_CIPHER = 2;
constexpr uint32_t NATIVE_KEY = 1;
constexpr uint32_t NATIVE_KDF = 1;
constexpr uint32_t NATIVE_DB = 1;
constexpr uint32_t NATIVE_HMAC = 1;
constexpr uint32_t NATIVE_AUTH = 1;
constexpr uint32_t NATIVE_RATE = 0;
constexpr uint32_t NATIVE_RESOLV = 1;
constexpr uint32_t NATIVE_ECC = 0;
constexpr uint32_t NATIVE_FEC = 0;


class FENRIR_LOCAL Lib
{
public:
    using list_t =typename std::vector<std::pair<Dynamic_Type, uint16_t>> (*)();
    using get_shared_t = std::shared_ptr<Dynamic> (*) (Event::Loop *const loop,
                                                Loader *const loader,
                                                Random *const rnd,
                                                const std::shared_ptr<Lib> self,
                                                const Dynamic_Type type,
                                                const uint16_t id);

    Lib (const std::string &file)
    {
        Fenrir_List_Plugins = nullptr;
        Fenrir_Get_Plugin = nullptr;
        char *error;
        // major, minor version -- Major for incompatibilities, minor for additions
        std::tuple<uint32_t, uint32_t> (*Fenrir_check_version) ();

        handle = dlopen (file.c_str(), RTLD_LAZY);
        if (!handle)
            return;

        dlerror();    // Clear any existing error
        Fenrir_check_version = reinterpret_cast<decltype (Fenrir_check_version)>
                                    (dlsym (handle, "Fenrir_check_version"));
        error = dlerror();
        if (error != nullptr || Fenrir_check_version == nullptr) {
            dlclose (handle);
            return;
        }
        auto version = (*Fenrir_check_version) ();
        if (std::get<0> (version) != 0 || std::get<1> (version) != 0) {
            // Fernrir version not supported
            dlclose (handle);
            return;
        }
        Fenrir_Get_Plugin = reinterpret_cast<decltype (Fenrir_Get_Plugin)>
                                        (dlsym (handle, "Fenrir_Get_Plugin"));
        Fenrir_List_Plugins = reinterpret_cast<decltype (Fenrir_List_Plugins)>
                                        (dlsym (handle, "Fenrir_List_Plugins"));

        // nothing implemented?
        if (Fenrir_List_Plugins == nullptr || Fenrir_Get_Plugin == nullptr) {
            Fenrir_Get_Plugin   = nullptr;
            Fenrir_List_Plugins = nullptr;
            dlclose (handle);
        }
    }

    Lib (list_t list, get_shared_t get)
        : handle (nullptr), Fenrir_List_Plugins (list), Fenrir_Get_Plugin (get)
    {}
    Lib () = delete;
    Lib (const Lib&) = delete;
    Lib& operator= (const Lib&) = delete;
    Lib (Lib&&) = default;
    Lib& operator= (Lib&&) = default;
    ~Lib ()
    {
        if (handle != nullptr)
            dlclose (handle);
    }

    explicit operator bool() const
    {
        return Fenrir_List_Plugins != nullptr && Fenrir_Get_Plugin != nullptr;
    }

    inline static std::shared_ptr<Lib> native()
    {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wexit-time-destructors"
        static auto native = std::make_shared<Lib> (&list_native,
                                                            &get_shared_native);
        #pragma clang diagnostic pop
        return native;
    }

    template<typename T>
    std::vector<typename T::ID> list_supported()
    {
        std::vector<typename T::ID> ret;
        std::vector<std::pair<Dynamic_Type, uint16_t>> tmp =
                                                    (*Fenrir_List_Plugins)();
        const constexpr auto type = Dynamic_Type_From_Template<T>();
        for (auto pair : tmp) {
            if (pair.first == type)
                ret.emplace_back (typename T::ID {pair.second});
        }
        return ret;
    }
    template<typename T>
    std::shared_ptr<T> get_plugin (Event::Loop *const loop,
                                                Loader *const loader,
                                                Random *const rnd,
                                                std::shared_ptr<Lib> self,
                                                const typename T::ID id)
    {
        const constexpr auto type = Dynamic_Type_From_Template<T>();
        auto ret = (*Fenrir_Get_Plugin) (loop, loader, rnd, self, type,
                                                    static_cast<uint16_t> (id));
        return std::static_pointer_cast<T> (ret);
    }

private:
    void *handle = nullptr;
    list_t Fenrir_List_Plugins = nullptr;
    get_shared_t Fenrir_Get_Plugin = nullptr;

    static std::vector<std::pair<Dynamic_Type, uint16_t>> list_native();
    static std::shared_ptr<Dynamic> get_shared_native (Event::Loop *const loop,
                                                Loader *const loader,
                                                Random *const rnd,
                                                const std::shared_ptr<Lib> self,
                                                const Dynamic_Type type,
                                                const uint16_t id);
};

} // namespace Impl
} // namespace Fenrir__v1
