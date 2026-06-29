/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "RegisterItemDelegate.h"
#include "RegisterTableModel.h"

#include <QColor>
#include <QPainter>

QWidget* RegisterItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (m_model)
        m_model->setEditingIndex(index);  // 编辑开始:上报真实编辑 index
    return editor;
}

void RegisterItemDelegate::destroyEditor(QWidget* editor, const QModelIndex& index) const
{
    if (m_model)
        m_model->clearEditingIndex();      // 编辑结束:解除保护
    QStyledItemDelegate::destroyEditor(editor, index);
}

void RegisterItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);  // 正常底色 + 文字 + 选中态
    if (!m_model) return;

    const QColor overlay = m_model->flashOverlay(index.row(), index.column());
    if (overlay.isValid())
        painter->fillRect(option.rect, overlay);          // 半透明叠加(alpha 已含衰减)
}
