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
#include "Fenrir/v1/data/AS_list.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/resolve/Resolver.hpp"
#include "Fenrir/v1/plugin/Loader.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
#include "Fenrir/v1/util/endian.hpp"
#include "Fenrir/v1/util/Z85.hpp"
#include <algorithm>
#include <arpa/nameser.h>
#include <atomic>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unbound.h>


namespace Fenrir__v1 {
namespace Impl {

namespace Resolve {
    class DNSSEC;
    static constexpr Resolver::ID dnssec_plugin_id { 1 };
    static constexpr std::chrono::milliseconds poll_rate {500};
} // namespace Resolve

namespace Event {

// unbound needs
class FENRIR_LOCAL DNSSEC final : public Plugin_Timer
{
public:
    size_t _counter;
    std::mutex _mtx;

    DNSSEC() = delete;
    DNSSEC (Loop *const loop, std::weak_ptr<Dynamic> resolver)
        : Plugin_Timer (loop, std::move(resolver)), _counter (0)
        {}
    static std::shared_ptr<DNSSEC> mk_shared (Loop *const loop,
                                                std::weak_ptr<Dynamic> resolver)
    {
        auto e = std::make_shared<DNSSEC> (loop, std::move(resolver));
        e->_ourselves = e;
        return e;
    }
};

static void dnssec_ev_add (std::shared_ptr<DNSSEC> ev, Loop *const loop)
{
    std::unique_lock<std::mutex> lock (ev->_mtx);
    FENRIR_UNUSED (lock);
    if (ev->_counter == 0)
        loop->start (ev, Impl::Resolve::poll_rate, Event::Repeat::YES);
    ++ev->_counter;
}

static void dnssec_ev_del (std::shared_ptr<DNSSEC> ev, Loop *const loop)
{
    std::unique_lock<std::mutex> lock (ev->_mtx);
    FENRIR_UNUSED (lock);
    --ev->_counter;
    if (ev->_counter == 0)
        loop->deactivate (ev);
}

} // namespace Event

class Random;

namespace Resolve {

class FENRIR_LOCAL DNSSEC final : public Resolver
{
public:
    DNSSEC (Event::Loop *const loop, Loader *const loader, Random *const rnd)
        : Resolver (std::shared_ptr<Lib> (nullptr), loop, loader, rnd),
            id_next (0), common (nullptr)
        {}
    DNSSEC() = delete;
    DNSSEC (const DNSSEC&) = default;
    DNSSEC& operator= (const DNSSEC&) = default;
    DNSSEC (DNSSEC &&) = default;
    DNSSEC& operator= (DNSSEC &&) = default;
    ~DNSSEC();
    ID id() const override
        { return dnssec_plugin_id; }
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override;

    bool initialize() override;
    void stop() override;
    Impl::Error resolv_async (std::shared_ptr<Event::Resolve> request) override;

private:
    typedef void ub_fenrir;	// struct ub_ctx is an incomplete type. workaround

    class FENRIR_LOCAL request_data;

    class FENRIR_LOCAL dnssec_data
    {
    public:
        Event::Loop *const loop;
        Loader *const loader;
        Random *const rnd;
        // struct ub_context, with custom deleter
        std::unique_ptr<ub_fenrir, void(*)(ub_fenrir*)> ctx;
        std::mutex tracker_mtx;
        std::map<int32_t, std::unique_ptr<request_data>> tracker;
        std::shared_ptr<Event::DNSSEC> _dnssec_polling;

        dnssec_data (Event::Loop *const _loop, Loader *const _loader,
                        Random *const _rnd, std::weak_ptr<Dynamic> resolver)
            :loop (_loop), loader (_loader), rnd (_rnd),
                        ctx (nullptr, &unbound_cleanup), _dnssec_polling (
                                    Event::DNSSEC::mk_shared (loop, resolver))
            {}
    };

    class FENRIR_LOCAL request_data
    {
    public:
        request_data (std::shared_ptr<dnssec_data> _common,
                            std::shared_ptr<Event::Resolve> _ev, int32_t _id)
            : common (_common), returned_ev (_ev), id (_id) {}
        std::shared_ptr<dnssec_data> common;
        std::shared_ptr<Event::Resolve> returned_ev;
        int32_t id;
    };

    std::atomic<uint32_t> id_next;
    std::shared_ptr<dnssec_data> common;

    // custom deleter to run ub_ctx_delete once we have finished.
    static void callback (void* mydata, int err, struct ub_result* result);
    static void unbound_cleanup (ub_fenrir *p);
    static AS_list parse_dnssec_binary (const std::vector<uint8_t> &raw,
                                        std::shared_ptr<dnssec_data> &_common);
};


FENRIR_INLINE bool DNSSEC::initialize()
{
    int32_t retval;

    common = std::make_shared<DNSSEC::dnssec_data>(_loop, _loader, _rnd, _self);
    // custom deleter to run ub_ctx_delete once we have finished.
    common->ctx = std::unique_ptr<ub_fenrir, void(*)(ub_fenrir*)> (
                                    static_cast<ub_fenrir *> (ub_ctx_create()),
                                                            unbound_cleanup);

    // handle async in dedicated thread, not in dedicated process.
    ub_ctx_async (static_cast<struct ub_ctx *> (common->ctx.get()), true);

    if(common->ctx == nullptr)
        return false;
    if (ub_ctx_async (static_cast<struct ub_ctx *> (common->ctx.get()), 1))
        return false;
    // read /etc/resolv.conf for DNS proxy settings
    retval = ub_ctx_resolvconf (static_cast<struct ub_ctx *> (
                            common->ctx.get()), FENRIR_DIR_CONF "/resolv.conf");
    if (retval != 0)
        return false;
    // we could load up /etc/hosts, but it doesn't provide public keys,
    // and we don't do non-secure resolving.

    // read public keys for DNSSEC verification
    retval = ub_ctx_add_ta_file (static_cast<struct ub_ctx *> (
                    common->ctx.get()), FENRIR_DIR_CONF "/Fenrir/dnssec.key");
    if (retval != 0)
        return false;
    return true;
}

FENRIR_INLINE DNSSEC::~DNSSEC()
    { stop(); }

FENRIR_INLINE void DNSSEC::stop()
{
    assert (common != nullptr);

    std::unique_lock<std::mutex> lock (common->tracker_mtx);
    FENRIR_UNUSED (lock);
    for (auto &req : common->tracker) {
        int32_t ret = ub_cancel (
                            static_cast<struct ub_ctx *> (common->ctx.get()),
                                                                req.second->id);
        if (ret == 0) {
            // ret == 0 if callback has not been executed yet
            common->tracker.erase (req.second->id);
        }
    }
    // trigger all stopped events
    ub_process (static_cast<struct ub_ctx *> (common->ctx.get()));
    dnssec_ev_del (common->_dnssec_polling, _loop);
    _loop->del (common->_dnssec_polling);
}

FENRIR_INLINE void DNSSEC::parse_event (std::shared_ptr<Event::Plugin_Timer> ev)
{
    // libunbound uses a thread to parse everything, and you can provide
    // a callback to get the results back.
    // ..but when is the callback called? it turns out you have to poll
    // for that. -.-'
    FENRIR_UNUSED (ev);
    // process and call the callback
    ub_process (static_cast<struct ub_ctx *> (common->ctx.get()));
}


FENRIR_INLINE Impl::Error DNSSEC::resolv_async (
                                        std::shared_ptr<Event::Resolve> request)
{
    // send the request
    int32_t retval;

    if (common == nullptr)
        return Impl::Error::INITIALIZATION;
    int32_t id = 0;
    auto data = std::make_unique<request_data> (common, request, id);
    auto data_raw = data.get();
    std::vector<char> dnssec_fqdn;
    dnssec_fqdn.reserve (9 + request->_fqdn.size());
    constexpr std::array<char, 8> prepend {{ '_','f','e','n','r','i','r', '.'}};
    std::copy (prepend.begin(), prepend.end(), std::back_inserter(dnssec_fqdn));
    std::copy (request->_fqdn.begin(), request->_fqdn.end(),
                                            std::back_inserter(dnssec_fqdn));
    dnssec_fqdn.push_back ('\0');

    common->tracker_mtx.lock();
    retval = ub_resolve_async (static_cast<struct ub_ctx *> (
                                                        common->ctx.get()),
                                                        dnssec_fqdn.data(),
                                                        T_TXT, C_IN, data_raw,
                                                        DNSSEC::callback,
                                                        &data_raw->id);
    dnssec_ev_add (common->_dnssec_polling, _loop);
    common->tracker.emplace (id, std::move(data));
    common->tracker_mtx.unlock();

    if (retval != 0)
        return Impl::Error::RESOLVE_NOT_FOUND;
    return Impl::Error::NONE;
}

FENRIR_INLINE void DNSSEC::callback (void* mydata, int err,
                                                    struct ub_result* result)
{
    std::unique_ptr<request_data> data {static_cast<request_data *> (mydata)};
    assert (data != nullptr && "Fenrir::DNSSEC data nullptr");
    dnssec_ev_del (data->common->_dnssec_polling, data->common->loop);
    data->returned_ev->_last_resolver = dnssec_plugin_id;

    if (err != 0 || result->nxdomain || !result->havedata ||// have something
                        result->bogus || !result->secure) { // and be secure
        // new event, resolution failed.
        if (result->bogus || !result->secure) {
            data->returned_ev->_as._err = Impl::Error::RESOLVE_NOT_FENRIR;
        } else {
            data->returned_ev->_as._err = Impl::Error::RESOLVE_NOT_FOUND;
        }
        data->common->loop->add_work (std::move(data->returned_ev));
        std::unique_lock<std::mutex> lock (data->common->tracker_mtx);
        FENRIR_UNUSED (lock);
        data->common->tracker.erase (data->id);
        return;
    }

    // FIXME: multiple TXT records
    std::vector<std::vector<uint8_t>> raw {std::vector<uint8_t>()};
    for (uint32_t i = 0; i < 1; ++i) {
        int32_t read_length = 0;
        raw[i].reserve (1 + static_cast<size_t> (result->len[i]));
        while (read_length != result->len[i]) {
            uint8_t len = static_cast<uint8_t> (result->data[i][read_length]);
            ++read_length;
            auto *p = result->data[i] + read_length;
            std::copy (p, p + len, std::back_inserter (raw[i]));
            read_length += len;
        }
        raw[i].push_back ('\0');
    }

    for (uint32_t i = 0; i < raw.size(); ++i) {
        AS_list tmp = parse_dnssec_binary (raw[i], data->common);
        // only keep the first result without errors
        if (tmp._err == Impl::Error::NONE) {
            // todo: insert other info (?)
            data->returned_ev->_as = std::move(tmp);
            break;
        }
    }
    ub_resolve_free (result);
    data->common->loop->add_work (std::move(data->returned_ev));

    std::unique_lock<std::mutex> lock (data->common->tracker_mtx);
    FENRIR_UNUSED (lock);
    data->common->tracker.erase (data->id);
}

FENRIR_INLINE void DNSSEC::unbound_cleanup (ub_fenrir *p)
{
    // custom deleter to run ub_ctx_delete once we have finished.
    if (p == nullptr)
        return;
    // deletethe context
    ub_ctx_delete (static_cast<struct ub_ctx *> (p));
}

FENRIR_INLINE AS_list DNSSEC::parse_dnssec_binary (
                                        const std::vector<uint8_t> &raw,
                                        std::shared_ptr<dnssec_data> &_common)
{
    // format of parsed data:
    // TXT record:
    // v=Fenrir1 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrst
    // uvwxyz!#$%&()*+-;<=>?@^_`{|}~
    // ecc..
    // raw, base85-decoded:
    // 1 byte: number of ip structs. each struct:
    //		* bit 0-1: (most significant) ip type: 00 == ipv4, 01 = ipv6
    //		* bit 2-4: (middle) priority (fallback)
    //		* bit 5-7: (less significant) server preference probability
    //		* 2 bytes: udp port
    //		* X bytes: ip address (length delendent on type)
    // 2 bytes: length of supported authentication algorithms
    // Y * 2 bytes: list of authentication algorithms
    // 1 byte: number of public keys. for each:
    //		* 2 bytes: public key serial
    //		* 2 byte: public key type
    //		* W bytes: public key (dependent on type)
    //
    // This function does not return errorrs. it adds data to the result
    // only if everything is ok.
    AS_list result;
    result._err = Impl::Error::WRONG_INPUT;

    if (raw.size() < 28)
        return result;
    auto tmp = raw.begin();
    // now skip: "(whitespace)v=Fenrir1(whitespace)"
    while (tmp != raw.end() && (*tmp == ' ' || *tmp == '\t'))
        ++tmp;
    if ((raw.end() - tmp) < 28)
        return result;

    constexpr std::array<uint8_t, 10> fenrir = { "v=Fenrir1" };
    if (!std::equal (tmp, tmp + 9, fenrir.begin()))
        return result;
    tmp += 9;

    while (tmp != raw.end() && (*tmp == ' ' || *tmp == '\t'))
        ++tmp;
    if ((raw.end() - tmp) < 18)
        return result;

    // now tmp points to something we can decode. trim it.
    const uint8_t *end = raw.data() + raw.size() - 1;
    while (*end == '\0' || *end == '\n' || *end == '\r' ||
                                                *end == ' ' || *end == '\t') {
        --end;
    }
    ++end;

    gsl::span<const uint8_t> encoded {&*tmp, end};
    std::vector<uint8_t> decoded (z85_decoded_size (encoded), 0);
    gsl::span<uint8_t> dec_span {decoded};

    if (!Impl::z85_decode (encoded, dec_span))
        return result;

    // we can now parse the decoded data. format above.
    // server ips
    tmp = decoded.begin();
    uint8_t auth_servers = *tmp;
    ++tmp;
    for (uint8_t i = 0; i < auth_servers; ++i) {
        if (decoded.end() - tmp <= 3) {
            result._servers.clear();
            return result;
        }
        uint8_t flags = *(tmp++);
        uint16_t port = l_to_h<uint16_t> (*reinterpret_cast<const uint16_t *> (
                                                                        &*tmp));
        tmp += sizeof(port);
        bool ipv4;
        switch (flags >> 6) {
        case 0:
            ipv4 = true;
            break;
        case 1: // ipv6
            ipv4 = false;
            break;
        default:
            result._servers.clear();
            return result;
        }
        if (decoded.end() - tmp <= (ipv4 ? 4 : 16)) {
            result._servers.clear();
            return result;
        }
        result._servers.emplace_back (IP (&*tmp, !ipv4), port,
                                            (flags >> 3) & 0x07, flags  & 0x07);
        tmp += (ipv4 ? 4 : 16);
    }
    // server auths
    if (decoded.end() - tmp <= 3) {
        result._servers.clear();
        return result;
    }
    const uint16_t auth_len = l_to_h<uint16_t> (
                            *reinterpret_cast<const uint16_t *> (tmp.base()));
    tmp += sizeof(auth_len);
    if (static_cast<size_t>(decoded.end() - tmp) <= sizeof(uint16_t) +
                                                auth_len * sizeof(uint16_t)) {
        result._servers.clear();
        return result;
    }
    result._auth_protocols.reserve (auth_len);
    for (uint16_t idx = 0; idx < auth_len; ++idx) {
        const auto auth_alg = l_to_h<Crypto::Auth::ID> (
                            *reinterpret_cast<const Crypto::Auth::ID*>(&*tmp));
        tmp += sizeof(auth_alg);
        result._auth_protocols.push_back (auth_alg);
    }
    // public keys
    if (decoded.end() - tmp <= 6) {
        result._servers.clear();
        return result;
    }
    const uint16_t keys = l_to_h<uint16_t> (
                            *reinterpret_cast<const uint16_t *> (tmp.base()));
    tmp += sizeof(keys);
    result._key.reserve (keys);
    for (uint8_t idx = 0; idx < keys; ++idx) {
        if (decoded.end() - tmp <= 5) {
            result._servers.clear();
            result._auth_protocols.clear();
            result._key.clear();
            return result;
        }
        const auto key_id = l_to_h<Crypto::Key::Serial> (
                    *reinterpret_cast<const Crypto::Key::Serial *> (&*tmp));
        tmp += sizeof(key_id);
        const auto key_type = l_to_h<Crypto::Key::ID> (
                            *reinterpret_cast<const Crypto::Key::ID*> (&*tmp));
        tmp += sizeof(key_type);
        auto key = _common->loader->get_shared<Crypto::Key> (key_type);
        if (key == nullptr)
            continue; // unsupported key.
        const auto pubkey_len = key->get_publen();
        if (decoded.end() - tmp < pubkey_len) {
            result._servers.clear();
            result._auth_protocols.clear();
            result._key.clear();
            return result;
        }
        const gsl::span<const uint8_t> key_span (tmp.base(),
                                                    tmp.base() + pubkey_len);
        if (!key->init (key_span)) {
            result._servers.clear();
            result._auth_protocols.clear();
            result._key.clear();
            return result;
        }
        result._key.emplace_back (key_id, std::move(key));
    }
    if (result._key.size() == 0) {
        result._servers.clear();
        result._auth_protocols.clear();
        return result;
    }
    result._err = Impl::Error::NONE;
    return result;
}


} // namespace Resolve
} // namespace Impl
} // namespaece Fenrir__v1
