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
#include <functional>

// 状态栏单段:紧凑值 + 悬停详情。纯展示,由 StatusBarController 填内容。
// 可见值随状态频繁更新;详情惰性构建——仅在鼠标悬浮时经 provider 生成,
// 既避免对不可见 tooltip 每帧做功,又保证每次显示的都是当时的实时状态。
// value/detail 均支持简单 HTML(彩色圆点、两列表);由控制器统一格式化。
class StatusSegment : public QLabel
{
    Q_OBJECT

public:
    explicit StatusSegment(QWidget* parent = nullptr);

    // 设置紧凑显示值(热路径,仅 setText)
    void setValue(const QString& value);
    // 设置详情构建器(装一次即可);悬浮时调用,返回空则不显示 tooltip
    void setDetailProvider(std::function<QString()> provider);

protected:
    bool event(QEvent* e) override;   // 拦 QEvent::ToolTip,悬浮时才构建详情

private:
    std::function<QString()> m_detailProvider;
};

#endif // STATUSSEGMENT_H
