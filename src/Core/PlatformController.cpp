/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "Core/PlatformController.h"

#include "Core/RegisterStore.h"
#include "SimulationPlatform.h"
#include <QThread>
#include <QMetaObject>
#include <cmath>

PlatformController::PlatformController(RegisterStore* store,
                                      SimulationPlatform* platform,
                                      int powerXY,
                                      int powerD,
                                      QObject* parent)
    : QObject(parent), m_store(store), m_platform(platform),
      m_powerXY(powerXY), m_powerD(powerD)
{
}

void PlatformController::setUnitPowers(int powerXY, int powerD)
{
    m_powerXY = powerXY;
    m_powerD  = powerD;
}

double PlatformController::divisorXY() const
{
    return std::pow(10.0, m_powerXY);
}

double PlatformController::divisorD() const
{
    return std::pow(10.0, m_powerD);
}

void PlatformController::moveAbsolute(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (QThread::currentThread() == this->thread()) { doMoveAbsolute(xAddr, yAddr, aAddr, fmt); return; }
    QMetaObject::invokeMethod(this, [=]{ doMoveAbsolute(xAddr, yAddr, aAddr, fmt); }, Qt::QueuedConnection);
}

void PlatformController::moveRelative(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (QThread::currentThread() == this->thread()) { doMoveRelative(xAddr, yAddr, aAddr, fmt); return; }
    QMetaObject::invokeMethod(this, [=]{ doMoveRelative(xAddr, yAddr, aAddr, fmt); }, Qt::QueuedConnection);
}

void PlatformController::writeCurrentPos(int xAddr, int yAddr, int aAddr, NumFormat fmt, Platform which)
{
    if (QThread::currentThread() == this->thread()) { doWriteCurrentPos(xAddr, yAddr, aAddr, fmt, which); return; }
    QMetaObject::invokeMethod(this, [=]{ doWriteCurrentPos(xAddr, yAddr, aAddr, fmt, which); }, Qt::BlockingQueuedConnection);
}

void PlatformController::doMoveAbsolute(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (!m_store || !m_platform) return;
    Pose target;
    if (fmt == NumFormat::Int32) {
        const double dXY = divisorXY(), dD = divisorD();
        target.x = static_cast<double>(m_store->GetInt32(xAddr)) / dXY;
        target.y = static_cast<double>(m_store->GetInt32(yAddr)) / dXY;
        target.angleDeg = static_cast<double>(m_store->GetInt32(aAddr)) / dD;
    } else {
        target.x = m_store->GetFloat(xAddr);
        target.y = m_store->GetFloat(yAddr);
        target.angleDeg = m_store->GetFloat(aAddr);
    }
    m_platform->moveAbsolute(target);
}

void PlatformController::doMoveRelative(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (!m_store || !m_platform) return;
    Pose delta;
    if (fmt == NumFormat::Int32) {
        const double dXY = divisorXY(), dD = divisorD();
        delta.x = static_cast<double>(m_store->GetInt32(xAddr)) / dXY;
        delta.y = static_cast<double>(m_store->GetInt32(yAddr)) / dXY;
        delta.angleDeg = static_cast<double>(m_store->GetInt32(aAddr)) / dD;
    } else {
        delta.x = m_store->GetFloat(xAddr);
        delta.y = m_store->GetFloat(yAddr);
        delta.angleDeg = m_store->GetFloat(aAddr);
    }
    m_platform->moveRelative(delta);
}

void PlatformController::doWriteCurrentPos(int xAddr, int yAddr, int aAddr, NumFormat fmt, Platform which)
{
    if (!m_store || !m_platform) return;
    const Pose p = m_platform->pose(which);
    if (fmt == NumFormat::Int32) {
        const double dXY = divisorXY(), dD = divisorD();
        m_store->SetInt32(xAddr, static_cast<int32_t>(p.x * dXY));
        m_store->SetInt32(yAddr, static_cast<int32_t>(p.y * dXY));
        m_store->SetInt32(aAddr, static_cast<int32_t>(p.angleDeg * dD));
    } else {
        m_store->SetFloat(xAddr, static_cast<float>(p.x));
        m_store->SetFloat(yAddr, static_cast<float>(p.y));
        m_store->SetFloat(aAddr, static_cast<float>(p.angleDeg));
    }
}
