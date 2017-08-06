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

#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/plugin/Loader.hpp"

namespace Fenrir__v1 {
namespace Impl {

inline Impl::Error Loader::load_directory (const std::string &dir)
{
    DIR *search_dir;
    struct dirent *sysdir_d;
    int32_t ret;
    struct stat buf;

    ret = stat(dir.c_str(), &buf);
    if (ret)
        return Impl::Error::NO_SUCH_FILE;
    if (!S_ISDIR(buf.st_mode))
        return Impl::Error::NO_SUCH_FILE; // not a dir.

    if ((search_dir = opendir(dir.c_str())) == nullptr)
        return Impl::Error::INITIALIZATION;
    while ((sysdir_d = readdir(search_dir)) != nullptr) {
        if (sysdir_d->d_ino == 0)
            // why? don't remember... always did it
            continue;
        if (strlen(sysdir_d->d_name) <= 4)
            continue; // we need at least 4 chars: 'X.so' as a filename
        if (strncmp(sysdir_d->d_name + strlen(sysdir_d->d_name) - 3, ".so", 3))
            continue; // check that the filename ends with .so
        load_file (sysdir_d->d_name); // ignore errors?
    }
    return Impl::Error::NONE;
}

} // namespace Impl
} // namespace Fenrir__v1
