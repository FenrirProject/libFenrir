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
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/resolve/Resolver.hpp"
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>

struct ub_result;

namespace Fenrir__v1 {
namespace Impl {

namespace Resolve {
    class DNSSEC;
    static constexpr Resolver::ID dnssec_plugin_id { 1 };
    static constexpr std::chrono::milliseconds poll_rate {500};
} // namespace Resolve

class Lib;
class Loader;

namespace Event {

class Loop;

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
                                                std::weak_ptr<Dynamic> resolver);
};

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


} // namespace Resolve
} // namespace Impl
} // namespaece Fenrir__v1
