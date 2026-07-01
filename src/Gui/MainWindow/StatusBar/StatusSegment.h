/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef STATUSSEGMENT_H
#define STATUSSEGMENT_H

#include <QLabel>

// 状态栏单段:紧凑值 + 悬停详情。纯展示,由 StatusBarController 填内容。
// value/detail 均支持简单 HTML(彩色圆点、多行 <br>);由控制器统一格式化。
class StatusSegment : public QLabel
{
    Q_OBJECT

public:
    explicit StatusSegment(QWidget* parent = nullptr);

    // 设置紧凑显示值与悬停详情(detail 为空则清空 tooltip)
    void set(const QString& value, const QString& detail = QString());
};

#endif // STATUSSEGMENT_H
