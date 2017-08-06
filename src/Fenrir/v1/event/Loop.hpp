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
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/event/IO.hpp"
#include "Fenrir/v1/net/Direction.hpp"
#include <condition_variable>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

// NOTE:
// The idea is: every event is a shared_ptr.
// The event has a shared_ptr to itself. Yes, circular.
// On event deletion we create a new event, type "Delete", with a
// pointer to the event we have to delete, and we break the loop in the
// event. This should avoid all race conditions possible with libevent
// since we need to delete the libevent structures from the same thread
// libevent uses for the loop.

struct ev_loop; // avoid including ev.h here

namespace Fenrir__v1 {
namespace Impl {
namespace Event {

enum class Repeat : uint8_t { YES, NO };

// loop management and event queue management
class FENRIR_LOCAL Loop
{
public:
    Loop();
    ~Loop();
    Loop (const Loop&) = delete;
    Loop& operator=(const Loop&) = delete;
    Loop (Loop&&) = delete;
    Loop& operator=(Loop&&) = delete;

    // true == all ok;
    explicit operator bool() const;
    void loop();

    std::shared_ptr<Base> wait();
    std::shared_ptr<Base> timed_wait (const std::chrono::microseconds usec);
    // FIXME : move to stop_loop, and move "deactivate" to "stop"
    void stop(); // does not destroy the queue.

    void start (std::shared_ptr<IO> ev);
    void start (std::shared_ptr<Timer> ev, const std::chrono::microseconds time,
                                        const Repeat rep);
    void update (std::shared_ptr<Base> ev);
    void deactivate (std::shared_ptr<Base> ev);
    void del (std::shared_ptr<Base> ev);

    void add_work (std::shared_ptr<Base> arg);
private:
    std::thread event_thread;
    struct ev_loop *ev_base;

    std::mutex loop_mtx, work_mtx;
    std::condition_variable cond;
    std::deque<std::shared_ptr<Base>> work;
    ev_timer _runev_hack;
    bool _keep_running;

    static void event_loop (Loop *const loop); // what the thread will execute
};

} // namespace Event
} // namespace Impl
} // namespace Fenrir__v1

