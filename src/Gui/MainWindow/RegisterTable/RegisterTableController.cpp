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
#include <QHeaderView>

RegisterTableController::RegisterTableController(QTableView* view, RegisterStore* store, QWidget* parent)
    : QObject(parent)
    , m_view(view)
    , m_store(store)
    , m_parentWidget(parent)
    , m_model(nullptr)
{
}

void RegisterTableController::initTable()
{
    if (!m_view) return;

    m_model = new RegisterTableModel(m_store, m_parentWidget, this);
    m_view->setModel(m_model);

    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setAlternatingRowColors(true);
    m_view->setItemDelegate(new RegisterItemDelegate(m_model, m_view));

    connect(m_model, &RegisterTableModel::flashTick, this, [this]
    {
        m_view->viewport()->update();
    });
}

void RegisterTableController::setWatches(const QVector<DeviceAddress>& items)
{
    if (m_model) m_model->setWatches(items);
}

void RegisterTableController::setDataType(RegisterDataType type)
{
    if (m_model) m_model->setDataType(type);
}

void RegisterTableController::setNumberBase(bool hex)
{
    if (m_model) m_model->setNumberBase(hex);
}

void RegisterTableController::retranslateHeaders()
{
    if (m_model)
        m_model->refreshHeaders();
}
