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

enum  optionIndex {UNKNOWN, HELP, ENCODE, DECODE, F_IN, F_OUT, PAD};
const option::Descriptor usage[] =
{
 {UNKNOWN,  0, "", "", Arg::Unknown, "USAGE:  {-e|-d} -i [IN] -o [OUT] [-p]\n"},
 {HELP,     0, "h", "help", Arg::None, "  -h --help\tThis help."},
 {ENCODE,   0, "e", "encode", Arg::None, "-e --encode"},
 {DECODE,   0, "d", "decode", Arg::None, "-d --decode"},
 {PAD,      0, "p", "padding", Arg::None, "-p --padding\n"
                "\tRemove restriction on input/output size\n"
                "\tNote: when encoded with padding can not be decoded without\n"
                "\tThis option puts an extra character at the beginning." },
 {F_IN,     0, "i", "input", Arg::String, "-i --input [FILE]\n"
                                            "\tinput file ('-' for stdin)"},
 {F_OUT,    0, "o", "output", Arg::String, "-o --output [FILE]\n"
                                            "\toutput file ('-' for stdout)"},
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
    if ((options[F_IN].count() != 1 || options[F_OUT].count() != 1)) {
        std::cerr << "Err: need exactly one input and output\n";
        option::printUsage (std::cout, usage);
        return 1;
    }

    std::ifstream in_f;
    std::istream *in;
    std::ofstream out_f;
    std::ostream *out;

    ssize_t size = 1024;
    if (0 == strncmp ("-", options[F_IN].arg, 2)) {
        in = &std::cin;
    } else {
        in_f = std::ifstream (options[F_IN].arg,
                                            std::ios::ate | std::ios::binary);
        if (!in_f.is_open()) {
            std::cerr << "Can't open input file\n";
            return 1;
        }
        size = in_f.tellg();
        in_f.seekg (0);
        in = &in_f;
    }
    if (0 == strncmp ("-", options[F_OUT].arg, 2)) {
        out = &std::cout;
    } else {
        out_f = std::ofstream (options[F_OUT].arg,  std::ios::binary);
        if (!out_f.is_open()) {
            std::cerr << "Can't open output file\n";
            return 1;
        }
        out = &out_f;
    }

    std::vector<uint8_t> tmp;
    tmp.reserve (static_cast<size_t> (size));
    std::array<uint8_t, 1024> buf;
    while (!in->eof()) {
        in->read (reinterpret_cast<char*> (buf.data()), buf.size());
        if (in->gcount() > 0)
            std::copy (buf.data(), buf.data() + in->gcount(),
                                                    std::back_inserter (tmp));
    }

    if (options[ENCODE].count() > 0) {
        std::vector<uint8_t> encoded;
        if (options[PAD].count() == 0) {
            encoded = std::vector<uint8_t> (
                                Fenrir__v1::Impl::z85_encoded_size_no_header (
                                                                tmp.size()));
            if (!Fenrir__v1::Impl::z85_encode_no_header (tmp, encoded)) {
                std::cerr << "Err: can't encode\n";
                return 1;
            }
        } else {
            encoded = std::vector<uint8_t> (Fenrir__v1::Impl::z85_encoded_size (
                                                                tmp.size()));
            if (!Fenrir__v1::Impl::z85_encode (tmp, encoded)) {
                std::cerr << "Err: can't encode\n";
                return 1;
            }
        }
        out->write (reinterpret_cast<char*>(encoded.data()),
                                        static_cast<ssize_t> (encoded.size()));

    } else if (options[DECODE].count() > 0) {
        std::vector<uint8_t> decoded;
        if (options[PAD].count() == 0) {
            decoded = std::vector<uint8_t> (
                                Fenrir__v1::Impl::z85_decoded_size_no_header (
                                                                tmp.size()));
            if (!Fenrir__v1::Impl::z85_decode_no_header (tmp, decoded)) {
                std::cerr << "Err: can't decode\n";
                return 1;
            }
        } else {
            decoded = std::vector<uint8_t> (Fenrir__v1::Impl::z85_decoded_size (
                                                                        tmp));
            if (!Fenrir__v1::Impl::z85_decode (tmp, decoded)) {
                std::cerr << "Err: can't decode\n";
                return 1;
            }
        }
        out->write (reinterpret_cast<char*>(decoded.data()),
                                        static_cast<ssize_t> (decoded.size()));
    }

    return 0;
}
