// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: (C) 2022 Lars Toenning <dev@ltoenning.de>

/**
 * @file
 * @brief Unit tests for SGTimer and SGTimerQueue
 */

#include <cstdlib>

#include <simgear/misc/test_macros.hxx>

#include "event_mgr.hxx"

void testSGTimer() {
    int call_counter = 0;
    SGTimer timer;
    timer.callback = [&call_counter]() { ++call_counter; };
    timer.repeat = false;
    timer.running = false;
    timer.interval = 0.1;
    timer.name = "TestTimer";

    // Check single run
    timer.run();
    SG_CHECK_EQUAL(call_counter, 1);

    // Check multiple runs
    call_counter = 0;
    for (int i = 0; i < 5; i++) {
        timer.run();
    }
    SG_CHECK_EQUAL(call_counter, 5);

    // run shouldn't have side effects on members
    SG_CHECK_EQUAL(timer.repeat, false);
    SG_CHECK_EQUAL(timer.running, false);
    SG_CHECK_EQUAL(timer.interval, 0.1);
    SG_CHECK_EQUAL(timer.name, "TestTimer");
}

void testSGTimerQueueClear() {
    SGTimerQueue queue;
    int call_counter = 0;
    std::map<std::string, double> stats;

    auto timer = std::make_unique<SGTimer>();
    timer->callback = [&call_counter]() { ++call_counter; };
    timer->repeat = true;
    timer->interval = 0.5;

    queue.insert(std::move(timer), 1);

    SG_CHECK_EQUAL(call_counter, 0);
    queue.update(0.5, stats);
    SG_CHECK_EQUAL(call_counter, 0);
    queue.update(0.5, stats);
    SG_CHECK_EQUAL(call_counter, 1);
    queue.update(0.4, stats);
    SG_CHECK_EQUAL(call_counter, 1);
    queue.update(0.1, stats);
    SG_CHECK_EQUAL(call_counter, 2);
    queue.update(42.0, stats);
    SG_CHECK_EQUAL(call_counter, 3);

    queue.clear();
    queue.update(0.6, stats);
    SG_CHECK_EQUAL(call_counter, 3);
}

void testSGTimerQueueRemoveByName() {
    SGTimerQueue queue;
    int call_counter = 0;
    std::map<std::string, double> stats;

    auto timer = std::make_unique<SGTimer>();
    timer->callback = [&call_counter]() { ++call_counter; };
    timer->name = "TestTimer1";
    timer->repeat = true;
    timer->interval = 1;
    queue.insert(std::move(timer), 0);

    SG_CHECK_EQUAL(call_counter, 0);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 1);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 2);
    SG_CHECK_EQUAL(queue.removeByName("TestTimer1"), true);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 2);
}

void testSGTimerQueueOneShot() {
    SGTimerQueue queue;
    int call_counter = 0;
    std::map<std::string, double> stats;

    auto timer = std::make_unique<SGTimer>();
    timer->callback = [&call_counter]() { ++call_counter; };
    timer->name = "TestTimer1";
    timer->repeat = false;
    timer->interval = 1;
    queue.insert(std::move(timer), 0);

    SG_CHECK_EQUAL(call_counter, 0);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 1);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 1);
    SG_CHECK_EQUAL(queue.removeByName("TestTimer1"), false);
    queue.update(1.0, stats);
    SG_CHECK_EQUAL(call_counter, 1);
}

int main(int argc, char *argv[]) {
    testSGTimer();
    testSGTimerQueueClear();
    testSGTimerQueueRemoveByName();
    testSGTimerQueueOneShot();

    return EXIT_SUCCESS;
}
