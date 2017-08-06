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
#include "Fenrir/v1/data/IP.hpp"
#include "Fenrir/v1/net/Link_defs.hpp"
#include "Fenrir/v1/util/endian.hpp"
#include <cassert>
#include <dirent.h>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <utility>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

namespace Event {
class Read;
}

class FENRIR_LOCAL Socket
{
public:
    std::shared_ptr<Event::Read> sock_ev;

    Socket (const Link_ID id);

    Socket () = delete;
    Socket (const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket (Socket&&) = delete;
    Socket& operator=(Socket&&) = delete;
    ~Socket ();

    explicit operator bool() const
        { return buffer.capacity() != 0; }

    int64_t write (const std::vector<uint8_t> &input, const Link_ID link);
    std::tuple<Link_ID, std::vector<uint8_t>> read();
                                                // can be blocking,
                                                // we check the socket anyway
    int32_t get_fd()
        { return fd; }
    Link_ID id() const
        { return Link_ID ({ip, port}); }
private:
    const IP ip;
    const UDP_Port port;
    std::mutex mtx;
    std::vector<uint8_t> buffer;

    int32_t fd = -1; // file descriptor for event monitoring

    uint32_t find_mtu();
};

FENRIR_INLINE Socket::~Socket()
{
    close(fd);
    errno = 0; //close sets errno on early error when fd has not been set
}

FENRIR_INLINE Socket::Socket (const Link_ID id)
    : ip (id.ip()), port (id.udp_port())
{
    // create a UDP socket, either to send or receive, ipv4 or ipv6
    struct sockaddr_in s_in4;
    struct sockaddr_in6 s_in6;

    uint32_t bufsize = find_mtu();
    if (bufsize == 0)
        return;
    buffer.reserve (bufsize);
    if (buffer.capacity() != bufsize) {
        buffer = std::vector<uint8_t>(); // force memory release
        return;
    }
    if (!ip.ipv6) {
        if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            buffer = std::vector<uint8_t>(); // force memory release
            return;
        }
        (memset_ptr) (reinterpret_cast<char *> (&s_in4), 0, sizeof(s_in4));
        s_in4.sin_family = PF_INET;
        s_in4.sin_port = h_to_b<uint16_t> (static_cast<uint16_t> (port));
        s_in4.sin_addr.s_addr = ip.ip.v4.s_addr;

        if (bind(fd, reinterpret_cast<struct sockaddr *> (&s_in4),
                                                            sizeof(s_in4))) {
            buffer = std::vector<uint8_t>(); // force memory release
            return;
        }
    } else {
        if ((fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            buffer = std::vector<uint8_t>(); // force memory release
            return;
        }
        (memset_ptr) (reinterpret_cast<char *> (&s_in6), 0, sizeof(s_in6));
        s_in6.sin6_family = PF_INET6;
        s_in6.sin6_port = h_to_b<uint16_t> (static_cast<uint16_t> (port));
        s_in6.sin6_addr = ip.ip.v6;

        if (bind(fd, reinterpret_cast<struct sockaddr *> (&s_in6),
                                                            sizeof(s_in6))) {
            buffer = std::vector<uint8_t>(); // force memory release
            return;
        }
    }
}

FENRIR_INLINE int64_t Socket::write (const std::vector<uint8_t> &input,
                                                        const Link_ID link)
{
    // ipv4/v6 respected and either udp communication or raw ip communication.
    assert (link.ip().ipv6 == ip.ipv6 &&
                    ((link.udp_port() == UDP_Port(0) && port == UDP_Port (0)) ||
                    (port != UDP_Port(0) && link.udp_port() != UDP_Port(0))));
    // TODO: raw IP support
    ssize_t ret;
    union {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
        struct sockaddr raw;
    } sock;

    std::unique_lock<std::mutex> lock (mtx, std::defer_lock);
    if (link.ip().ipv6) {
        sock.v6.sin6_family = AF_INET6;
        sock.v6.sin6_port = h_to_b<uint16_t> (static_cast<uint16_t> (
                                                            link.udp_port()));
        sock.v6.sin6_flowinfo = 0;
        sock.v6.sin6_addr = link.ip().ip.v6;
        sock.v6.sin6_scope_id = 0;
        lock.lock();
        ret = sendto(fd, input.data(), input.size(), 0, &sock.raw,
                                                            sizeof(sock.v6));
    } else {
        sock.v4.sin_family = AF_INET;
        sock.v4.sin_port = h_to_b<uint16_t> (static_cast<uint16_t> (
                                                            link.udp_port()));
        sock.v4.sin_addr = link.ip().ip.v4;
        lock.lock();
        ret = sendto(fd, input.data(), input.size(), 0, &sock.raw,
                                                            sizeof(sock.v4));
    }
    return ret;
}

FENRIR_INLINE std::tuple<Link_ID, std::vector<uint8_t>> Socket::read()
{
    // TODO: raw ip support
    std::vector<uint8_t> data;
    Link_ID from;
    uint32_t from_len;
    ssize_t len;
    union {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
        struct sockaddr raw;
    } sock;


    from_len = sizeof(sock);
    std::unique_lock<std::mutex> lock (mtx, std::defer_lock);
    if (ip.ipv6) {
        from_len = sizeof(struct sockaddr_in6);
        lock.lock();
        len = recvfrom (fd, reinterpret_cast<char *> (buffer.data()),
                                    buffer.capacity(), 0, &sock.raw, &from_len);
        if (from_len != sizeof(struct sockaddr_in))
            return {from, std::move(data)};
        from.ip().ip.v6 = sock.v6.sin6_addr;
        from.ip().ipv6 = true;
    } else {
        from_len = sizeof(struct sockaddr_in);
        lock.lock();
        len = recvfrom (fd, reinterpret_cast<char *> (buffer.data()),
                                    buffer.capacity(), 0, &sock.raw, &from_len);
        if (from_len != sizeof(struct sockaddr_in))
            return {from, std::move(data)};
        from.ip().ip.v4 = sock.v4.sin_addr;
        from.ip().ipv6 = false;
    }
    if (len <= 0) {
        lock.unlock();
        (memset_ptr) (&from.ip().ip.v6, 0, sizeof(from.ip().ip.v6));
        return {from, std::move(data)};
    }
    data.reserve (static_cast<size_t> (len));
     // TODO: can we make it zero-copy?
    std::copy (buffer.data(), buffer.data() + len, std::back_inserter (data));
    lock.unlock();
    if (ip.ipv6) {
        from.udp_port() = UDP_Port (b_to_h<uint16_t>(sock.v6.sin6_port));
    } else {
        from.udp_port() = UDP_Port (b_to_h<uint16_t>(sock.v4.sin_port));
    }
    return {from, std::move(data)};
}

// TODO: This is Linux only!
// we'd also like to support *BSD and MacOSX...
// should we #define everything? :(
// TODO: pass address so we can check the correct interface.

FENRIR_INLINE uint32_t Socket::find_mtu ()
{
    // find the *BIGGEST* mtu possible for all interfaces.
    // since we can in theory receive from everyone, and we have no control
    // on what they send
    // /sys/class/net/$interface/mtu is the bytes.
    // TODO: check for MTU changes.

    DIR *sys_dir;
    struct dirent *sysdir_d;
    FILE *mtu_file;
    char *path = nullptr, *reall_p;
    uint32_t pathlen, mtu = 0, tmp_mtu;
    uint64_t ret;

    if ((sys_dir = opendir("/sys/class/net/")) == nullptr)
        return 0;
    while ((sysdir_d = readdir(sys_dir)) != nullptr) {
        if (!strncmp (sysdir_d->d_name, ".", 2) ||
                                        !strncmp (sysdir_d->d_name, "..", 3)) {
            continue;
        }
        pathlen = 16 + static_cast<uint32_t> (strlen (sysdir_d->d_name) + 4);
        reall_p = reinterpret_cast<char *> (realloc(path, pathlen));
        if (reall_p == nullptr) {
            free (path);
            closedir (sys_dir);
            return 0;
        }
        path = reall_p;
        snprintf (path, pathlen, "/sys/class/net/%s/mtu", sysdir_d->d_name);
        mtu_file = fopen (path, "r");
        if (mtu_file == nullptr) {
            free (path);
            closedir (sys_dir);
            continue;
        }
        // path has a double usage: we also use it to read the file.
        (memset_ptr) (path, 0, pathlen);
        ret = fread (path, 1, pathlen, mtu_file);
        fclose (mtu_file);
        if (ret == 0) {
            free (path);
            closedir (sys_dir);
            continue;
        }
        path[ret] = '\0'; // just to be sure...
        tmp_mtu = static_cast<uint32_t> (strtoul (path, &reall_p, 10));
        // check to see that the mtu is he only thing here.
        if (tmp_mtu == 0) {
            free (path);
            closedir (sys_dir);
            continue;
        }
        if (*reall_p != '\0' && *reall_p != '\n') {
            free (path);
            closedir (sys_dir);
            return 0;
        }
        if (mtu < tmp_mtu)
            mtu = tmp_mtu;
    }
    return mtu;
}

} // namespace Impl
} // namespace Fenrir__v1
