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

class RegisterTableManager;

// 寄存器表编辑委托:
// - setModelData 用 Qt 设计的提交钩子处理单元格编辑,取代易出问题的 commitData 外部槽;
//   校验/写回交给 RegisterTableManager::commitEdit(非法即不写入模型 → 单元格自动保留原值)。
// - paint 在正常绘制之上叠加闪烁淡出色(由 RegisterTableManager::flashOverlay 提供)。
class RegisterItemDelegate : public QStyledItemDelegate
{
public:
    explicit RegisterItemDelegate(RegisterTableManager* manager, QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_manager(manager)
    {
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    RegisterTableManager* m_manager;
};

#endif // REGISTERITEMDELEGATE_H
