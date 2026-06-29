/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "RegisterItemDelegate.h"
#include "RegisterTableManager.h"

#include <QLineEdit>
#include <QPainter>

void RegisterItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const
{
    // 不调用基类(基类会把原始文本写回模型),全权交给 commitEdit 决策写/不写
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit || !m_manager)
    {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }
    m_manager->commitEdit(index.row(), index.column(), lineEdit->text());
}

void RegisterItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);   // 正常底色 + 文字 + 选中态
    if (!m_manager) return;

    const QColor overlay = m_manager->flashOverlay(index.row(), index.column());
    if (overlay.isValid())
        painter->fillRect(option.rect, overlay);          // 半透明叠加(alpha 已含衰减)
}
