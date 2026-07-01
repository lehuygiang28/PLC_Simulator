/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "StatusSegment.h"

StatusSegment::StatusSegment(QWidget* parent)
    : QLabel(parent)
{
    setTextFormat(Qt::RichText);

    // 悬停详情皮肤:深色气泡(仅作用于本部件的 QToolTip,不改全局主题)
    setStyleSheet(
        "QLabel { padding: 0px 4px; }"   // 段内左右内边距(带样式表后 setContentsMargins 失效,须写进 padding)
        "QToolTip {"
        "  color: #e5e7eb;"
        "  background-color: #1f2937;"
        "  border: 1px solid #374151;"
        "  padding: 8px 10px;"
        "}");
}

void StatusSegment::set(const QString& value, const QString& detail)
{
    setText(value);
    setToolTip(detail);
}
