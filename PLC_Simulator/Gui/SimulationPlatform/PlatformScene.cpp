/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "PlatformScene.h"
#include <QGuiApplication>
#include <QScreen>

PlatformScene::PlatformScene(QObject* parent)
    : QObject(parent)
{
    m_virtualMark.visible = false;  // 原 showVirtualMarkCheckBox 默认 false
    recomputeScale();
}

void PlatformScene::recomputeScale()
{
    if (m_screenRatio <= 0.0)
        return;
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;   // headless 环境下 primaryScreen 可能为空
    // 现取主屏宽度,不缓存:换屏/改分辨率后再次调参即可跟上(原 m_scale = m_ScreenWidth / m_Ratio)
    const double screenWidthPx = screen->availableGeometry().width();
    m_pixelsPerMm = screenWidthPx / m_screenRatio;
}

PlatformItem& PlatformScene::itemRef(Platform which)
{
    return which == Platform::Base ? m_base : m_live;
}

const PlatformItem& PlatformScene::platform(Platform which) const
{
    return which == Platform::Base ? m_base : m_live;
}

void PlatformScene::moveAbsolute(const Pose& target, Platform which)
{
    PlatformItem& it = itemRef(which);
    if (it.pose.x == target.x && it.pose.y == target.y && it.pose.angleDeg == target.angleDeg)
        return;  // 无变化不发信号(等价原轮询的去重)
    it.pose = target;
    emit poseChanged(which, it.pose);
    emit changed();
}

void PlatformScene::moveRelative(const Pose& delta, Platform which)
{
    if (delta.x == 0.0 && delta.y == 0.0 && delta.angleDeg == 0.0)
        return;
    PlatformItem& it = itemRef(which);
    it.pose.x += delta.x;
    it.pose.y += delta.y;
    it.pose.angleDeg += delta.angleDeg;
    emit poseChanged(which, it.pose);
    emit changed();
}

Pose PlatformScene::pose(Platform which) const
{
    return platform(which).pose;
}

void PlatformScene::setPlatformVisible(Platform which, bool visible)
{
    itemRef(which).visible = visible;
    emit changed();
}

void PlatformScene::setBaseMarkPose(const Pose& pose) { m_baseMark.pose = pose; emit changed(); }
void PlatformScene::setLiveMarkPose(const Pose& pose) { m_liveMark.pose = pose; emit changed(); }
void PlatformScene::setVirtualMarkOffset(double x, double y) { m_virtualMark.pose.x = x; m_virtualMark.pose.y = y; emit changed(); }
void PlatformScene::setBaseMarkFollows(bool follows) { m_baseMark.followsPlatform = follows; emit changed(); }
void PlatformScene::setLiveMarkFollows(bool follows) { m_liveMark.followsPlatform = follows; emit changed(); }
void PlatformScene::setBaseMarkVisible(bool visible) { m_baseMark.visible = visible; emit changed(); }
void PlatformScene::setLiveMarkVisible(bool visible) { m_liveMark.visible = visible; emit changed(); }
void PlatformScene::setVirtualMarkVisible(bool visible) { m_virtualMark.visible = visible; emit changed(); }

void PlatformScene::setSceneParams(double markCenterDistance, double screenRatio)
{
    m_markCenterDistance = markCenterDistance;
    m_screenRatio = screenRatio;
    recomputeScale();
    emit changed();
}

namespace {
constexpr auto kMarkCenterDistance = "mark_center_distance";
constexpr auto kScreenRatio        = "screen_ratio";
} // namespace

QVariantMap PlatformScene::toVariantMap() const
{
    QVariantMap m;
    m[kMarkCenterDistance] = m_markCenterDistance;
    m[kScreenRatio]        = m_screenRatio;
    return m;
}

void PlatformScene::fromVariantMap(const QVariantMap& m)
{
    // 缺字段时退回当前值(= 默认 20.0 / 200.0),并走 setSceneParams 重算缩放+发 changed
    setSceneParams(m.value(kMarkCenterDistance, m_markCenterDistance).toDouble(),
                   m.value(kScreenRatio, m_screenRatio).toDouble());
}

Pose PlatformScene::baseMarkFinalPose() const
{
    if (m_baseMark.followsPlatform)
        return { m_base.pose.x + m_baseMark.pose.x,
                 m_base.pose.y + m_baseMark.pose.y,
                 m_base.pose.angleDeg + m_baseMark.pose.angleDeg };
    return m_baseMark.pose;
}

Pose PlatformScene::liveMarkFinalPose() const
{
    if (m_liveMark.followsPlatform)
        return { m_live.pose.x + m_liveMark.pose.x,
                 m_live.pose.y + m_liveMark.pose.y,
                 m_live.pose.angleDeg + m_liveMark.pose.angleDeg };
    return m_liveMark.pose;
}
