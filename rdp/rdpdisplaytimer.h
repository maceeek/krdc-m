/*
 * SPDX-FileCopyrightText: 2026 KRDC-M contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

// The RDP connection uses std::thread, without a Qt event loop. Own and join
// the debounce worker so neither the timer nor its callback outlive the display.
class RdpDisplayTimer
{
public:
    explicit RdpDisplayTimer(std::chrono::milliseconds interval = std::chrono::milliseconds(500))
        : m_interval(interval)
        , m_thread([this](std::stop_token token) {
            std::unique_lock lock(m_mutex);
            while (m_condition.wait(lock, token, [this] {
                return bool(m_callback);
            })) {
                const auto generation = m_generation;
                const auto deadline = m_deadline;
                if (m_condition.wait_until(lock, token, deadline, [this, generation] {
                        return generation != m_generation;
                    })) {
                    continue;
                }
                if (token.stop_requested()) {
                    break;
                }
                auto callback = std::exchange(m_callback, {});
                lock.unlock();
                callback();
                lock.lock();
            }
        })
    {
    }

    ~RdpDisplayTimer()
    {
        stop();
    }

    void start(std::function<void()> callback)
    {
        {
            std::lock_guard lock(m_mutex);
            m_callback = std::move(callback);
            m_deadline = std::chrono::steady_clock::now() + m_interval;
            ++m_generation;
        }
        m_condition.notify_all();
    }

    void stop()
    {
        if (m_thread.joinable()) {
            m_thread.request_stop();
            m_condition.notify_all();
            m_thread.join();
        }
    }

private:
    const std::chrono::milliseconds m_interval;
    std::mutex m_mutex;
    std::condition_variable_any m_condition;
    std::function<void()> m_callback;
    std::chrono::steady_clock::time_point m_deadline;
    unsigned long long m_generation = 0;
    std::jthread m_thread;
};
