/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef PLATFORMSCENE_H
#define PLATFORMSCENE_H

#include <QObject>
#include "PlatformTypes.h"

/**
 * @brief 模拟平台场景数据模型(单一数据源)
 *
 * 持有基准/实时平台与三种 Mark 的位姿、可见性、场景参数;不依赖任何 widget。
 * 任意数据变化发 changed() 供视图重绘;平台移动额外发 poseChanged() 供外部同步。
 */
class PlatformScene : public QObject
{
    Q_OBJECT
public:
    explicit PlatformScene(QObject* parent = nullptr);

    // ---- 平台移动(mm/度) ----
    void moveAbsolute(const Pose& target, Platform which = Platform::Live);
    void moveRelative(const Pose& delta, Platform which = Platform::Live);
    Pose pose(Platform which = Platform::Live) const;
    void setPlatformVisible(Platform which, bool visible);

    // ---- Mark 设置(控制面板驱动) ----
    void setBaseMarkPose(const Pose& pose);
    void setLiveMarkPose(const Pose& pose);
    void setVirtualMarkOffset(double x, double y);   // 虚拟 Mark 仅 x,y 偏移
    void setBaseMarkFollows(bool follows);
    void setLiveMarkFollows(bool follows);
    void setBaseMarkVisible(bool visible);
    void setLiveMarkVisible(bool visible);
    void setVirtualMarkVisible(bool visible);

    // ---- 场景参数 ----
    void setSceneParams(double markCenterDistance, double screenRatio);
    double markCenterDistance() const { return m_markCenterDistance; }
    double screenRatio() const { return m_screenRatio; }
    double pixelsPerMm() const { return m_pixelsPerMm; }

    // ---- 只读访问(供画布渲染) ----
    const PlatformItem& platform(Platform which) const;
    const MarkItem& baseMark() const { return m_baseMark; }
    const MarkItem& liveMark() const { return m_liveMark; }
    const MarkItem& virtualMark() const { return m_virtualMark; }
    Pose baseMarkFinalPose() const;   // 跟随基准平台后的终位
    Pose liveMarkFinalPose() const;   // 跟随实时平台后的终位

signals:
    void changed();                                   // 任意变化→重绘
    void poseChanged(Platform which, const Pose& pose); // 平台移动→外部同步

private:
    PlatformItem& itemRef(Platform which);
    void recomputeScale();

    PlatformItem m_base;
    PlatformItem m_live;
    MarkItem m_baseMark;
    MarkItem m_liveMark;
    MarkItem m_virtualMark;
    double m_markCenterDistance = 20.0;
    double m_screenRatio = 200.0;
    double m_pixelsPerMm = 0.0;
    double m_screenWidthPx = 0.0;
};

#endif // PLATFORMSCENE_H
