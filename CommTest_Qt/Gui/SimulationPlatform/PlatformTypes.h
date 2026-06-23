/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef PLATFORMTYPES_H
#define PLATFORMTYPES_H

// 位姿:位置(mm) + 朝向(度)
struct Pose {
    double x = 0.0;
    double y = 0.0;
    double angleDeg = 0.0;
};

// 平台选择
enum class Platform { Base, Live };

// 平台项:位姿 + 可见性
struct PlatformItem {
    Pose pose;
    bool visible = true;
};

// Mark 项:位姿 + 是否跟随平台 + 可见性
struct MarkItem {
    Pose pose;
    bool followsPlatform = true;
    bool visible = true;
};

// 绘图常量(单位 mm)
constexpr double MARK_RECT_WIDTH = 3.0;
constexpr double MARK_RECT_HEIGHT = 8.0;
constexpr double VIRTUAL_MARK_STATIC_X = 10.0;
constexpr double VIRTUAL_MARK_STATIC_Y = -10.0;

#endif // PLATFORMTYPES_H
