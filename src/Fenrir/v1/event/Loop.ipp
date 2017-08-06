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

#include "Fenrir/v1/event/Loop.hpp"
#include <cassert>
#include <ev.h>

namespace Fenrir__v1 {
namespace Impl {
namespace Event {

///////
// Loop
///////

FENRIR_INLINE Loop::Loop()
    : _keep_running (true)
{
    // TODO: if pthread -> pthread_atfork (call("ev_loop_fork"))
    ev_base = ev_default_loop (EVFLAG_AUTO | EVFLAG_FORKCHECK);
    if (ev_base == nullptr)
        return;
    // we need to be sure that the loop is run each millisecond
    // reason: test:
    //   add timer: every 5secs
    //   run loop
    //   add timer: every 0.1 secs
    // the second timers starts after the first has run at least once.
    // no amount of loop start/stop/update/suspend/resume
    // seems to work. sigh.
    // ... at least even like this it does not show up in my cpu.
    // ...but we also won't let the cpu stay in idle?
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
    // not needed due to hack in event_loop()
    //ev_timer_init (&_runev_hack, cb_running_hack, 0., 0.001);
    #pragma clang diagnostic pop
    //ev_timer_start (ev_base, &_runev_hack);
}

FENRIR_INLINE Loop::~Loop()
{
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    _keep_running = false;
    if (ev_base)
        ev_break (ev_base);
    event_thread.join();
    FENRIR_UNUSED (loop_lock);
    if (ev_base != nullptr) {
        ev_loop_destroy (ev_base);
        ev_base = nullptr;
    }
}

FENRIR_INLINE Loop::operator bool() const
{
    // true == all ok;
    return (ev_base != nullptr);
}

FENRIR_INLINE void Loop::loop()
{
    _keep_running = true;
    event_thread = std::thread (&event_loop, this);
}

FENRIR_INLINE void Loop::event_loop (Loop *loop)
{
    // FIXME: hack
    // figure out how libev really works.
    // apparently:
    // * we can not stop a loop if there are running events.
    // * we can not immediately add a timer if  it triggers before the
    //   previous minimum
    // so we run once, then manually sleep. high cpu usage.
    while (loop->_keep_running) {
        ev_run (loop->ev_base, EVRUN_NOWAIT);
        std::this_thread::sleep_for (std::chrono::milliseconds{1});
    }
}

FENRIR_INLINE void Loop::stop()
{
    _keep_running = false;
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    if (ev_base)
        ev_break (ev_base, EVBREAK_ALL);
}

FENRIR_INLINE std::shared_ptr<Base> Loop::wait()
{
    std::unique_lock<std::mutex> lock (work_mtx);
    if (work.size() != 0) {
        auto todo = work.front();
        work.pop_front();
        return todo;
    }
    cond.wait (lock);
    if (work.size() != 0) {
        auto todo = work.front();
        work.pop_front();
        return todo;
    }
    return nullptr;
}

FENRIR_INLINE std::shared_ptr<Base> Loop::timed_wait (
                                        const std::chrono::microseconds usec)
{
    std::unique_lock<std::mutex> lock (work_mtx);
    if (work.size() != 0) {
        auto todo = work.front();
        work.pop_front();
        return todo;
    }
    cond.wait_for (lock, usec);
    if (work.size() != 0) {
        auto todo = work.front();
        work.pop_front();
        return todo;
    }
    return nullptr;
}

FENRIR_INLINE void Loop::add_work (std::shared_ptr<Base> arg)
{
    std::unique_lock<std::mutex> lock (work_mtx);
    FENRIR_UNUSED(lock);
    work.push_back (arg);
    cond.notify_one();
}

FENRIR_INLINE void Loop::start (std::shared_ptr<IO> ev)
{
    if (ev->_status != Base::status::INITIALIZED)
        return;
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    switch (ev->_type) {
    case Type::READ:
        ev->_status = Base::status::STARTED;
        ev_io_start (ev_base, &ev->_io_ev);
        break;
    default:
        assert (false && "Fenrir: activated timer as IO");
        break;
    }
}

FENRIR_INLINE void Loop::start (std::shared_ptr<Timer> ev,
                                        const std::chrono::microseconds time,
                                        const Repeat rep)
{
    if (ev->_status != Base::status::INITIALIZED)
        return;
    double after = static_cast<double> (time.count()) / 1000000;
    double repeat;
    if (rep == Repeat::YES) {
        repeat = after;
        after = 0.;
    } else {
        repeat = 0.;
    }

    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
    switch (ev->_type) {
    case Type::KEEPALIVE:
        ev->_status = Base::status::STARTED;
        ev_timer_set (&std::static_pointer_cast<Keepalive> (ev)->_time_ev,
                                                                after, repeat);
        ev_timer_start (ev_base,
                        &std::static_pointer_cast<Keepalive> (ev)->_time_ev);
        break;
    case Type::SEND:
        ev->_status = Base::status::STARTED;
        ev_timer_set (&std::static_pointer_cast<Send> (ev)->_time_ev,
                                                                after, repeat);
        ev_timer_start (ev_base,&std::static_pointer_cast<Send> (ev)->_time_ev);
        break;
    case Type::CONNECT:
        ev->_status = Base::status::STARTED;
        ev_timer_set (&std::static_pointer_cast<Connect> (ev)->_time_ev,
                                                                after, repeat);
        ev_timer_start (ev_base,
                        &std::static_pointer_cast<Connect> (ev)->_time_ev);
        break;
    case Type::HANDSHAKE:
        ev->_status = Base::status::STARTED;
        ev_timer_set (&std::static_pointer_cast<Handshake> (ev)->_time_ev,
                                                                after, repeat);
        ev_timer_start (ev_base,
                        &std::static_pointer_cast<Handshake> (ev)->_time_ev);
        break;
    case Type::PLUGIN_TIMER:
        ev->_status = Base::status::STARTED;
        ev_timer_set (&std::static_pointer_cast<Plugin_Timer> (ev)->_time_ev,
                                                                after, repeat);
        ev_timer_start (ev_base,
                        &std::static_pointer_cast<Plugin_Timer> (ev)->_time_ev);
        break;
    case Type::READ:
        assert (false && "Fenrir loop: activated io as timer");
        break;
    }
    #pragma clang diagnostic pop
}

FENRIR_INLINE void Loop::update (std::shared_ptr<Base> ev)
{
    if (ev->_status != Base::status::STARTED)
        return;
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
    switch (ev->_type) {
    case Type::KEEPALIVE:
        ev_timer_again (ev_base,
                        &std::static_pointer_cast<Keepalive> (ev)->_time_ev);
        break;
    case Type::SEND:
        ev_timer_again (ev_base,&std::static_pointer_cast<Send> (ev)->_time_ev);
        break;
    case Type::CONNECT:
        ev_timer_again (ev_base,
                            &std::static_pointer_cast<Connect> (ev)->_time_ev);
        break;
    case Type::HANDSHAKE:
        ev_timer_again (ev_base,
                        &std::static_pointer_cast<Handshake> (ev)->_time_ev);
        break;
    case Type::PLUGIN_TIMER:
        ev_timer_again (ev_base,
                        &std::static_pointer_cast<Plugin_Timer> (ev)->_time_ev);
        break;
    case Type::READ:
        assert (false && "Fenrir loop: activated io as timer");
        break;
    }
    #pragma clang diagnostic pop
}

FENRIR_INLINE void Loop::deactivate (std::shared_ptr<Base> ev)
{
    if (ev->_status != Base::status::STARTED)
        return;
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    switch (ev->_type) {
    case Type::KEEPALIVE:
        ev_timer_stop (ev_base,
                        &std::static_pointer_cast<Keepalive> (ev)->_time_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    case Type::READ:
        ev_io_stop (ev_base, &std::static_pointer_cast<Read> (ev)->_io_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    case Type::SEND:
        ev_timer_stop (ev_base,&std::static_pointer_cast<Send> (ev)->_time_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    case Type::CONNECT:
        ev_timer_stop (ev_base,
                        &std::static_pointer_cast<Connect> (ev)->_time_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    case Type::HANDSHAKE:
        ev_timer_stop (ev_base,
                        &std::static_pointer_cast<Handshake> (ev)->_time_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    case Type::PLUGIN_TIMER:
        ev_timer_stop (ev_base,
                        &std::static_pointer_cast<Plugin_Timer> (ev)->_time_ev);
        ev->_status = Base::status::INITIALIZED;
        break;
    }
}

void Loop::del (std::shared_ptr<Base> ev)
{
    std::unique_lock<std::mutex> loop_lock (loop_mtx);
    FENRIR_UNUSED (loop_lock);
    if (ev->_status == Base::status::STARTED)
        deactivate (ev);
}


} // namespace Event
} // namespace Impl
} // namespace Fenrir__v1

