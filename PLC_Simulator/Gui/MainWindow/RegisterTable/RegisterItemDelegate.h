/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERITEMDELEGATE_H
#define REGISTERITEMDELEGATE_H

#include <QStyledItemDelegate>

class RegisterTableModel;

// 寄存器表委托:
// - createEditor/destroyEditor 向 model 上报「正在编辑的 index」,供刷新时跳过(编辑保护);
//   提交走 QStyledItemDelegate 默认实现 → model::setData(校验/写回收敛在那里)。
// - paint 在正常绘制之上叠加闪烁淡出色(由 RegisterTableModel::flashOverlay 提供)。
class RegisterItemDelegate : public QStyledItemDelegate
{
public:
    explicit RegisterItemDelegate(RegisterTableModel* model, QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_model(model)
    {
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void destroyEditor(QWidget* editor, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    RegisterTableModel* m_model;
};

#endif // REGISTERITEMDELEGATE_H
