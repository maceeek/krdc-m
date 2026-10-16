/*
 * SPDX-FileCopyrightText: 2025 Fabio Bas <ctrlaltca@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QSize>
#include <QTimer>
#include <freerdp/client/disp.h>

struct RdpContext;

#include "rdpdisplaytimer.h"

class RdpDisplay : public QObject
{
    Q_OBJECT

public:
    RdpDisplay(RdpContext *krdp, DispClientContext *dispctx);
    ~RdpDisplay();

    static UINT onDisplayControlCaps(DispClientContext *disp, UINT32 maxNumMonitors, UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB);

    bool sendResizeEvent(const QSize newSize);

private:
    RdpContext *m_krdp;
    DispClientContext *m_dispctx = nullptr;
    RdpDisplayTimer m_updateTimer;

    std::mutex m_layoutMutex;
    QSize m_lastSize;
    UINT16 m_lastSentDesktopOrientation = 0;
    UINT32 m_lastSentDesktopScaleFactor = 0;
    UINT32 m_lastSentDeviceScaleFactor = 0;

public Q_SLOTS:
    void onUpdateTimer();
};