/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QGroupBox>

/**
 * @brief 可折叠分组框
 *
 * 继承 QGroupBox 以便在 Qt Designer 中由普通 QGroupBox 提升(promote)而来。
 * 点击标题栏切换折叠:折叠时隐藏所有子控件并将高度收缩至标题行,
 * 由父布局自动重排(下方控件上移、带伸缩因子的控件撑大)。
 * 标题前以 ▾/▸ 指示展开/折叠状态。
 */
class CollapsibleGroupBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(QWidget* parent = nullptr);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    bool event(QEvent* e) override;

private:
    void refreshTitlePrefix();
    int  headerHeight() const;

    QString m_baseTitle;
    bool    m_collapsed = false;
    bool    m_titleCaptured = false;
};

#endif // COLLAPSIBLEGROUPBOX_H
