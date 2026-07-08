/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "StatusSegment.h"

#include <QEvent>
#include <QHelpEvent>
#include <QToolTip>

StatusSegment::StatusSegment(QWidget* parent)
    : QLabel(parent)
{
    setTextFormat(Qt::RichText);

    // 段内左右内边距(带样式表后 setContentsMargins 失效,须写进 padding)。
    // tooltip 气泡不在此覆盖,交由全局 qss 的主题化 QToolTip,自动跟随深浅主题。
    setStyleSheet("QLabel { padding: 0px 4px; }");
}

void StatusSegment::setValue(const QString& value)
{
    setText(value);
}

void StatusSegment::setDetailProvider(std::function<QString()> provider)
{
    m_detailProvider = std::move(provider);
}

bool StatusSegment::event(QEvent* e)
{
    // 悬浮时才构建详情:免去热路径对不可见 tooltip 的做功,且内容为当时实时状态
    if (e->type() == QEvent::ToolTip && m_detailProvider)
    {
        const QString detail = m_detailProvider();
        if (!detail.isEmpty())
        {
            QToolTip::showText(static_cast<QHelpEvent*>(e)->globalPos(), detail, this);
            return true;
        }
    }
    return QLabel::event(e);   // 无 provider / 空详情 → 默认(不显示)
}
