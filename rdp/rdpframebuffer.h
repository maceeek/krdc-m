/*
 * SPDX-FileCopyrightText: 2026 KRDC-M contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QImage>
#include <mutex>
#include <utility>

// GDI keeps a raw pointer into its working image. Publish an owned copy only
// after EndPaint; QImage's implicit sharing cannot protect writes through that
// raw pointer. The GUI retains an immutable snapshot, including across resize.
class RdpFrameBuffer
{
public:
    void publish(const QImage &image)
    {
        auto completed = image.copy();
        std::lock_guard lock(m_mutex);
        m_image = std::move(completed);
    }

    QImage snapshot() const
    {
        std::lock_guard lock(m_mutex);
        return m_image;
    }

private:
    mutable std::mutex m_mutex;
    QImage m_image;
};
