/*
 * SPDX-FileCopyrightText: 2026 KRDC-M contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "rdpdisplaytimer.h"
#include "rdpframebuffer.h"

#include <QTest>
#include <atomic>
#include <cstring>
#include <future>
#include <memory>

using namespace std::chrono_literals;

class RdpStreamTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void snapshotSurvivesDecodeAndResize()
    {
        RdpFrameBuffer frames;
        QImage working(32, 24, QImage::Format_RGB32);
        working.fill(Qt::red);
        frames.publish(working);
        const auto first = frames.snapshot();
        // Simulate GDI writing through a pointer it retained, bypassing COW.
        auto raw = working.bits();
        memset(raw, 0, working.sizeInBytes());
        QCOMPARE(first.pixelColor(0, 0), QColor(Qt::red));
        working = QImage(64, 48, QImage::Format_RGB32);
        working.fill(Qt::blue);
        frames.publish(working);
        QCOMPARE(first.size(), QSize(32, 24));
        QCOMPARE(first.pixelColor(31, 23), QColor(Qt::red));
        const auto second = frames.snapshot();
        QCOMPARE(second.size(), QSize(64, 48));
        QCOMPARE(second.pixelColor(63, 47), QColor(Qt::blue));
    }

    void timerCancelsOnDestruction()
    {
        std::atomic_bool called = false;
        {
            RdpDisplayTimer timer(100ms);
            timer.start([&called] {
                called = true;
            });
        }
        QTest::qWait(150);
        QVERIFY(!called.load());
    }

    void timerJoinsRunningCallback()
    {
        std::promise<void> entered;
        std::promise<void> release;
        auto released = release.get_future().share();
        auto timer = std::make_unique<RdpDisplayTimer>(1ms);
        timer->start([&] {
            entered.set_value();
            released.wait();
        });
        QCOMPARE(entered.get_future().wait_for(2s), std::future_status::ready);
        auto stopped = std::async(std::launch::async, [&] {
            timer.reset();
        });
        const auto status = stopped.wait_for(20ms);
        release.set_value();
        stopped.get();
        QCOMPARE(status, std::future_status::timeout);
    }

    void timerDeliversLatestResize()
    {
        std::atomic_int result = 0;
        RdpDisplayTimer timer(200ms);
        timer.start([&] {
            result = 1;
        });
        QTest::qWait(30);
        timer.start([&] {
            result = 2;
        });
        QTest::qWait(40);
        QCOMPARE(result.load(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(result.load(), 2, 1000);
    }
};
QTEST_GUILESS_MAIN(RdpStreamTest)
#include "rdpstreamtest.moc"
