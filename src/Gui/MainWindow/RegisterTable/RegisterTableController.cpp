/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "RegisterTableController.h"
#include "RegisterTableModel.h"
#include "RegisterItemDelegate.h"

#include <QTableView>

RegisterTableController::RegisterTableController(QTableView* view, RegisterStore* store, QWidget* parent)
    : QObject(parent)
    , m_view(view)
    , m_store(store)
    , m_parentWidget(parent)
    , m_model(nullptr)
{
}

void RegisterTableController::initTable(int rowCount, int colCount)
{
    if (!m_view) return;

    m_model = new RegisterTableModel(m_store, m_parentWidget, this);
    m_model->setDimensions(rowCount, colCount);
    m_view->setModel(m_model);

    // 装配视图:数据/表头来自 model,此处只配外观与交互
    for (int col = 0; col < colCount; ++col)
        m_view->setColumnWidth(col, 80);
    m_view->setAlternatingRowColors(true);
    // 编辑触发沿用 QAbstractItemView 默认(双击/F2),与迁移前 QTableWidget 一致,无需显式设置
    m_view->setItemDelegate(new RegisterItemDelegate(m_model, m_view));

    // 闪烁节拍 → 重绘 viewport(model 不持 view,经信号解耦)
    connect(m_model, &RegisterTableModel::flashTick, this, [this]
    {
        m_view->viewport()->update();
    });
}

void RegisterTableController::setDataType(RegisterDataType type)
{
    if (m_model) m_model->setDataType(type);
}

void RegisterTableController::setStartAddr(int startAddr)
{
    if (m_model) m_model->setStartAddr(startAddr);
}

void RegisterTableController::setNumberBase(bool hex)
{
    if (m_model) m_model->setNumberBase(hex);
}
