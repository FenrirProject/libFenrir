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
#include "Fenrir/Fenrir_v1_hdr.hpp"
#include <array>
#include <gsl/span>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>


//////
/// Command line parsing stuff. see "optionparser" documentation
//////

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

static option::ArgStatus Unknown(const option::Option& option, bool msg)
{
    if (msg)
        std::cerr << "Unknown option '" << option.name << "'\n";
    return option::ARG_ILLEGAL;
}

};

enum  optionIndex {UNKNOWN, HELP, CONFIG};
const option::Descriptor usage[] =
{
 {UNKNOWN,  0, "", "", Arg::Unknown, "USAGE: -c [FILENAME]\n"},
 {HELP,     0, "h", "help", Arg::None, "-h --help\tThis help."},
 {CONFIG,   0, "c", "config", Arg::None, "-c --config [FILE]"},
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

    if (options[CONFIG].count() != 1) {
        std::cerr << "Err: need a configuration file\n";
        option::printUsage (std::cout, usage);
        return 1;
    }

    Fenrir__v1::Impl::Handler FH;
    if (!FH) {
        std::cerr << "FH init\n";
        return 1;
    }
    Fenrir__v1::Impl::IP ip;
    ip.ipv6 = false;
    ip.ip.v4 = in_addr{0};
    Fenrir__v1::Impl::Link_ID link {{ip, Fenrir__v1::Impl::UDP_Port {31338}}};
    if (!FH.listen (link)) {
        std::cerr << "Err: can't listen\n";
        return 1;
    }

    std::vector<uint8_t> fqdn { 'f','e','n','r','i','r',
                                'p','r','o','j','e','c','t',
                                '.','c','o','m'};
    Fenrir__v1::Impl::Service_ID service {{{ 0,0,0,0,0,0,0,0,
                                            0,0,0,0,0,0,0,0 }}};
    FH.connect (fqdn, service);


    while (1) {
        FH.get_report();
    }

    return 0;
}
