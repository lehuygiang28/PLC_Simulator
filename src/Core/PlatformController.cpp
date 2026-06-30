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
#include <QMutexLocker>
#include <cmath>

PlatformController::PlatformController(RegisterStore* store,
                                      SimulationPlatform* platform,
                                      int powerXY,
                                      int powerD,
                                      QObject* parent)
    : QObject(parent), m_store(store), m_platform(platform),
      m_powerXY(powerXY), m_powerD(powerD)
{
    if (m_platform) {
        // seed 初值;之后任何来源改场景位姿都经 poseChanged 同步进镜像
        m_poseMirror[idx(Platform::Base)] = m_platform->pose(Platform::Base);
        m_poseMirror[idx(Platform::Live)] = m_platform->pose(Platform::Live);
        connect(m_platform, &SimulationPlatform::poseChanged,
                this, &PlatformController::onPoseChanged);
    }
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

void PlatformController::onPoseChanged(Platform which, const Pose& pose)
{
    QMutexLocker lk(&m_poseMutex);
    m_poseMirror[idx(which)] = pose;
}

Pose PlatformController::readPose(int xAddr, int yAddr, int aAddr, NumFormat fmt) const
{
    Pose p;
    if (fmt == NumFormat::Int32) {
        const double dXY = divisorXY(), dD = divisorD();
        p.x        = static_cast<double>(m_store->GetInt32(xAddr)) / dXY;
        p.y        = static_cast<double>(m_store->GetInt32(yAddr)) / dXY;
        p.angleDeg = static_cast<double>(m_store->GetInt32(aAddr)) / dD;
    } else {
        p.x        = m_store->GetFloat(xAddr);
        p.y        = m_store->GetFloat(yAddr);
        p.angleDeg = m_store->GetFloat(aAddr);
    }
    return p;
}

void PlatformController::moveAbsolute(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (!m_store || !m_platform) return;
    const Pose target = readPose(xAddr, yAddr, aAddr, fmt);   // 调用线程解析
    {
        QMutexLocker lk(&m_poseMutex);
        m_poseMirror[idx(Platform::Live)] = target;          // 同步预更新(保 Move→Write 时序)
    }
    renderMoveAbsolute(target);                              // 编组到 GUI 渲染
}

void PlatformController::moveRelative(int xAddr, int yAddr, int aAddr, NumFormat fmt)
{
    if (!m_store || !m_platform) return;
    const Pose delta = readPose(xAddr, yAddr, aAddr, fmt);
    {
        QMutexLocker lk(&m_poseMutex);
        Pose& live = m_poseMirror[idx(Platform::Live)];
        live.x += delta.x; live.y += delta.y; live.angleDeg += delta.angleDeg;
    }
    renderMoveRelative(delta);
}

void PlatformController::writeCurrentPos(int xAddr, int yAddr, int aAddr, NumFormat fmt, Platform which)
{
    if (!m_store) return;
    Pose p;
    {
        QMutexLocker lk(&m_poseMutex);
        p = m_poseMirror[idx(which)];      // 读镜像,任意线程,不再编组/阻塞
    }
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

void PlatformController::renderMoveAbsolute(const Pose& target)
{
    if (QThread::currentThread() == this->thread()) { m_platform->moveAbsolute(target); return; }
    QMetaObject::invokeMethod(this, [this, target]{ m_platform->moveAbsolute(target); }, Qt::QueuedConnection);
}

void PlatformController::renderMoveRelative(const Pose& delta)
{
    if (QThread::currentThread() == this->thread()) { m_platform->moveRelative(delta); return; }
    QMetaObject::invokeMethod(this, [this, delta]{ m_platform->moveRelative(delta); }, Qt::QueuedConnection);
}
