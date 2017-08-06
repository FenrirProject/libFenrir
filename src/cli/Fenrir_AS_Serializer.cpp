/*
 * Copyright (c) 2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of "libFenrir".
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

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "../../external/optionparser-1.4/optionparser.h"
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
#include "Fenrir/v1/util/Z85.hpp"
#include "Fenrir/v1/util/endian.hpp"
#include <arpa/inet.h>
#include <gsl/span>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>


//////
/// Command line parsing stuff. see "optionparser" documentation
//////

struct srv {
    uint8_t priority;
    uint8_t preference;
    uint16_t udp_port;
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } ip;
    bool ipv6;
};

static struct srv parse_srv (const char *str)
{
    struct srv server;
    server.priority = 0;
    server.preference = 0;
    // accepted format:
    // priority:preference:UDP_PORT:IP
    // [1-8]:[1-8]:UDP_PORT:IP
    if (str == nullptr)
        return server;
    char *p = const_cast<char*> (str);
    char *endptr;
    size_t priority = strtoul (p, &endptr, 10);
    if (p == endptr || priority == 0 || priority > 8)
        return server;
    if (*endptr != ':')
        return server;
    p = endptr + 1;
    size_t preference = strtoul (p, &endptr, 10);
    if (preference == 0 || preference > 8)
        return server;
    if (*endptr != ':')
        return server;
    p = endptr + 1;
    size_t udp_port = strtoul (p, &endptr, 10);
    if (p == endptr || udp_port > 65535)
        return server;
    server.udp_port = static_cast<uint16_t> (udp_port);
    if (*endptr != ':')
        return server;
    p = endptr + 1;
    server.ipv6 = false;
    auto ret = inet_pton (AF_INET, p, &server.ip.v4);
    if (ret != 1) {
        // maybe ipv6?
        server.ipv6 = true;
        ret = inet_pton (AF_INET6, p, &server.ip.v6);
        if (ret != 1)
            return server;
    }
    server.priority = static_cast<uint8_t> (priority);
    server.preference = static_cast<uint8_t> (preference);
    return server;
}

static std::vector<uint16_t> parse_auth (const char *str)
{
    std::vector<uint16_t> ret;
    char *p = const_cast<char*> (str);
    char *endptr;
    if (p == nullptr)
        return ret;
    while (*p != '\0') {
        size_t auth = strtoul (p, &endptr, 10);
        if (auth == 0 || auth > 65535) {
            ret.clear();
            break;
        }
        ret.push_back (static_cast<uint16_t> (auth));
        if (*endptr != '\0' && *endptr != ',') {
            ret.clear();
            break;
        }
        if (*endptr == '\0')
            break;
        p = endptr + 1;
    }
    return ret;
}

struct key {
    uint16_t serial;
    uint16_t type;
    std::vector<uint8_t> key;
};

static struct key parse_key (const char *str)
{
    struct key ret;
    ret.type = 0;

    // accepted format:
    // serial:type:key(z85)
    if (str == nullptr)
        return ret;
    char *p = const_cast<char*> (str);
    char *endptr;
    size_t serial = strtoul (p, &endptr, 10);
    if (serial > 65535)
        return ret;
    ret.serial = static_cast<uint16_t> (serial);
    if (*endptr != ':')
        return ret;
    p = endptr + 1;
    size_t type = strtoul (p, &endptr, 10);
    if (type < 1 || type > 65535)
        return ret;
    if (*endptr != ':')
        return ret;
    p = endptr + 1;
    if (*p == '\0')
        return ret;

    auto size = Fenrir__v1::Impl::z85_decoded_size (gsl::span<uint8_t> {
                                            reinterpret_cast<uint8_t*> (p),
                                            static_cast<ssize_t> (strlen(p))});
    if (size == 0)
        return ret;
    ret.key = std::vector<uint8_t> (size, 0);
    gsl::span<const uint8_t> in (reinterpret_cast<const uint8_t*>(p),
                                            static_cast<ssize_t> (strlen(p)));
    gsl::span<uint8_t> out {ret.key};
    if (!Fenrir__v1::Impl::z85_decode (in, out))
        return ret;
    ret.type = static_cast<uint16_t> (type);
    return ret;
}

struct Arg: public option::Arg {

static option::ArgStatus String (const option::Option& option, bool msg)
{
    if (option.arg == nullptr) {
        if (msg)
            std::cerr << "Option " << option.name << " needs a parameter\n";
        return option::ARG_ILLEGAL;
    }
    return option::ARG_OK;
}

static option::ArgStatus Srv (const option::Option& option, bool msg)
{
    auto server = parse_srv (option.arg);
    if (server.priority == 0) {
        if (msg)
            std::cerr << "Option " << option.name << " wrong format\n";
        return option::ARG_ILLEGAL;
    }
    return option::ARG_OK;
}

static option::ArgStatus Auth (const option::Option& option, bool msg)
{
    auto auths = parse_auth (option.arg);
    if (auths.size() == 0) {
        if (msg)
            std::cerr << "Option " << option.name << " wrong format\n";
        return option::ARG_ILLEGAL;
    }
    return option::ARG_OK;
}

static option::ArgStatus Pubkey (const option::Option& option, bool msg)
{
    auto k = parse_key (option.arg);
    if (k.type == 0) {
        if (msg)
            std::cerr << "Option " << option.name << " wrong format\n";
        return option::ARG_ILLEGAL;
    }
    return option::ARG_OK;
}

static option::ArgStatus Unknown(const option::Option& option, bool msg)
{
    if (msg)
        std::cerr << "Unknown option '" << option.name << "'\n";
    return option::ARG_ILLEGAL;
}

static option::ArgStatus Numeric (const option::Option& option, bool msg)
{
    char* endptr = nullptr;
    int64_t res = -1;
    if (option.arg != 0)
        res = strtol(option.arg, &endptr, 10);
    // NOTE: numeric arguments must be >= 0
    if (endptr != option.arg && *endptr == 0 && res >= 0)
        return option::ARG_OK;

    if (msg) {
        std::cerr << "ERR: Option '" << option.name <<
                                            "' requires a numeric argument\n";
    }
    return option::ARG_ILLEGAL;
}
};

enum  optionIndex {UNKNOWN, HELP, ENCODE, SERVER, AUTH, PUBKEY, DECODE, BINARY};
const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "", Arg::Unknown, "USAGE: {-e|-d} FILE [OPTIONS]\n"},
 {HELP,    0, "h", "help", Arg::None, "  -h --help\tThis help."},
 {UNKNOWN, 0, "", "", Arg::Unknown, "ENCODING parameters:"},
 {ENCODE,  0, "e", "encode", Arg::String, "-e --encode\t[OUTPUT]\n"
            "\tencode the next parameters, send base85 output where specified"},
 {SERVER,  0, "s", "server", Arg::Srv, "-s --server DATA "
                                "\tadd an authentication server\n"
                                "\tformat: \"[1-8]:[1-8]:UDP_PORT:IP\""},
 {AUTH,    0, "a", "auth", Arg::Auth, "-a --auth list"
                                "\tadd an authentication algorithm\n"
                                "\tformat list: \"1,2,3\""},
 {PUBKEY,  0, "p", "pubkey", Arg::Pubkey, "-p --pubkey "
                                "\tadd a public key\n"
                                "\tformat: \"serial:type:key(base85)\"\n"
                                "\texample: \"42:1:asdfasdf...\""},
 {UNKNOWN, 0, "", "", Arg::None, "DECODING parameters:"},
 {DECODE,  0, "d", "decode", Arg::String, "-d --decode\t[OUTPUT]\n"
                        "\tdecode get a base85 output where specified"},
 {BINARY, 0, "b", "binary", Arg::None, "-b --binary\toutput in binary"},
 {0,0,0,0,0,0}
};


int main (int argc, char **argv)
{
    auto arg_num   = argc - 1;
    char **arguments;
    if (argc <= 1) {
        // no arguments
        arguments = nullptr;
    } else {
        arguments = argv + 1;
    }

    option::Stats  stats (usage, arg_num, arguments);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, arg_num, arguments,
                                const_cast<option::Option *> (options.data()),
                                const_cast<option::Option *> ( buffer.data()));


    if (parse.error() || options[HELP].count() != 0) {
        option::printUsage (std::cout, usage);
        if (parse.error())
            return 1;
        return 0;
    }

    if ((options[ENCODE].count() > 0 && options[DECODE].count() > 0) ||
            (options[ENCODE].count() == 0 && options[DECODE].count() == 0)) {
        std::cerr << "Err: use either encode or decode\n";
        option::printUsage (std::cout, usage);
        return 1;
    }

    bool error = false;
    if (options[ENCODE].count() > 0) {
        if (options[AUTH].count() != 1) {
            std::cerr << "Err: need a single list of authentication "
                                                                "algorithms\n";
            error = true;
        }
        if (options[SERVER].count() == 0) {
            std::cerr << "Err: need one or more servers\n";
            error = true;
        }
        if (options[PUBKEY].count() == 0) {
            std::cerr << "Err: need one or more pubkeys\n";
            error = true;
        }
        if (options[BINARY].count() != 0) {
            std::cerr << "Err: \"binary\" option is only for decoding\n";
            error = true;
        }
    }
    bool binary = false;
    if (options[DECODE].count() > 0) {
        if (options[AUTH].count() != 0) {
            std::cerr << "Err: authentication algorithms are for encoding\n";
            error = true;
        }
        if (options[SERVER].count() != 0) {
            std::cerr << "Err: servers are for encoding\n";
            error = true;
        }
        if (options[PUBKEY].count() != 0) {
            std::cerr << "Err: pubkeys are for encoding\n";
            error = true;
        }
        if (options[BINARY].count() != 0)
            binary = true;
    }
    if (error)
        return 1;

    std::vector<srv> servers;
    std::vector<uint16_t> auths;
    std::vector<key> keys;
    if (options[ENCODE].count() > 0) {
        option::Option *opt = options[SERVER];
        for (ssize_t idx = 0; idx < options[SERVER].count(); ++idx) {
            servers.push_back (parse_srv(opt->arg));
            opt = opt->next();
        }
        auths = parse_auth (options[AUTH].arg);
        opt = options[PUBKEY];
        uint32_t k_size = 0;
        for (ssize_t idx = 0; idx < options[PUBKEY].count(); ++idx) {
            keys.push_back (parse_key(opt->arg));
            k_size += keys.rbegin()->key.size() + sizeof(uint16_t) + sizeof(uint16_t);
            opt = opt->next();
        }

        // tot_size should actually be slightly higer.
        size_t tot_size = sizeof(uint8_t) + sizeof(struct srv) * servers.size() +
                            sizeof(uint16_t) * (1 + auths.size()) +
                            sizeof(uint16_t) + k_size;
        std::vector<uint8_t> raw (tot_size, 0);;
        // raw, base85-decoded:
        // 1 byte: number of ip structs. each struct:
        //		* bit 0-1: (most significant) ip type: 00 == ipv4, 01 = ipv6
        //		* bit 2-4: (middle) priority (fallback)
        //		* bit 5-7: (less significant) server preference probability
        //		* 2 bytes: udp port
        //		* X bytes: ip address (length delendent on type)
        // 2 bytes: length of supported authentication algorithms
        // Y * 2 bytes: list of authentication algorithms
        // 2 byte: number of public keys. for each:
        //		* 2 bytes: public key serial
        //		* 2 byte: public key type
        //		* W bytes: public key (dependent on type)
        uint8_t *p = raw.data();
        *p = static_cast<uint8_t> (servers.size());
        ++p;
        // servers
        for (const auto &srv : servers) {
            uint8_t flags = 0;
            if (srv.ipv6)
                flags |= (1 << 6);
            flags |= (srv.priority << 3);
            flags |= srv.preference;
            *p = flags;
            ++p;
            uint16_t net_port = Fenrir__v1::Impl::h_to_l<uint16_t> (srv.udp_port);
            uint8_t *port = reinterpret_cast<uint8_t*> (&net_port);
            *(p++) = *(port++);
            *(p++) = *port;
            if (srv.ipv6) {
                const uint8_t *ip =
                                reinterpret_cast<const uint8_t*> (&srv.ip.v6);
                for (size_t idx = 0; idx < sizeof (srv.ip.v6); ++idx)
                    *(p++) = *(ip++);
            } else {
                const uint8_t *ip =
                                reinterpret_cast<const uint8_t*> (&srv.ip.v4);
                for (size_t idx = 0; idx < sizeof (srv.ip.v4); ++idx)
                    *(p++) = *(ip++);
            }
        }
        //auths
        uint16_t auth_len = Fenrir__v1::Impl::h_to_l<uint16_t> (
                                        static_cast<uint16_t> (auths.size()));
        uint8_t *alen = reinterpret_cast<uint8_t*> (&auth_len);
        *(p++) = *(alen++);
        *(p++) = *alen;
        for (size_t idx = 0; idx < auths.size(); ++idx) {
            auths[idx] = Fenrir__v1::Impl::h_to_l<uint16_t> (auths[idx]);
            uint8_t *tmp = reinterpret_cast<uint8_t*> (&auths[idx]);
            *(p++) = *(tmp++);
            *(p++) = *tmp;
        }
        // pubkeys
        uint16_t pub_len = Fenrir__v1::Impl::h_to_l<uint16_t> (keys.size());
        uint8_t *klen = reinterpret_cast<uint8_t*> (&pub_len);
        *(p++) = *(klen++);
        *(p++) = *klen;
        for (auto &pub : keys) {
            pub.serial = Fenrir__v1::Impl::h_to_l<uint16_t> (pub.serial);
            pub.type = Fenrir__v1::Impl::h_to_l<uint16_t> (pub.type);
            uint8_t *kser = reinterpret_cast<uint8_t*> (&pub.serial);
            *(p++) = *(kser++);
            *(p++) = *kser;
            uint8_t *ktype = reinterpret_cast<uint8_t*> (&pub.type);
            *(p++) = *(ktype++);
            *(p++) = *ktype;
            for (const uint8_t byte : pub.key)
                *(p++) = byte;
        }
        uint16_t data_size = static_cast<uint16_t> (p - raw.data());

        raw.resize (data_size);

        tot_size = Fenrir__v1::Impl::z85_encoded_size (raw.size());
        std::vector<uint8_t> encoded (tot_size, 0);
        gsl::span<uint8_t> enc {encoded};
        if (!Fenrir__v1::Impl::z85_encode (raw, enc)) {
            std::cerr << "Err encoding\n";
            return 1;
        }
        for (const auto byte : encoded)
            std::cout << byte;
        std::cout << "\n";
    }
    if (options[DECODE].count() > 0) {

    }

    return 0;
}
