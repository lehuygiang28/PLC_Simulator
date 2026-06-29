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
#include <QLineEdit>

#include "RegisterTableManager.h"

// 寄存器表编辑委托:用 Qt 设计的提交钩子 setModelData 处理单元格编辑,
// 取代易出问题的 commitData 外部槽。校验/写回交给 RegisterTableManager::commitEdit:
// 非法即"不写入模型" → 单元格自动保留原值;弹框在 commitEdit 内延迟到编辑器关闭后。
class RegisterItemDelegate : public QStyledItemDelegate
{
public:
    explicit RegisterItemDelegate(RegisterTableManager* manager, QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_manager(manager)
    {
    }

    // 编辑提交:不调用基类(基类会把原始文本写回模型),全权交给 commitEdit 决策写/不写
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit || !m_manager)
        {
            QStyledItemDelegate::setModelData(editor, model, index);
            return;
        }
        m_manager->commitEdit(index.row(), index.column(), lineEdit->text());
    }

private:
    RegisterTableManager* m_manager;
};

#endif // REGISTERITEMDELEGATE_H
