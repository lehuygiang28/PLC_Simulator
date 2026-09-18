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
#include <QEvent>
#include <algorithm>

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
    m_model->setGridDimensions(21, 10);
    m_view->setModel(m_model);

    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setAlternatingRowColors(true);
    m_view->setItemDelegate(new RegisterItemDelegate(m_model, m_view));

    connect(m_model, &RegisterTableModel::flashTick, this, [this]
    {
        m_view->viewport()->update();
    });

    m_view->viewport()->installEventFilter(this);

    connect(m_model, &RegisterTableModel::editingFinished, this, [this]
    {
        applyViewportDimensions();
    }, Qt::QueuedConnection);

    applyViewportDimensions();
}

bool RegisterTableController::eventFilter(QObject* obj, QEvent* ev)
{
    if (m_view && obj == m_view->viewport() && ev->type() == QEvent::Resize)
        applyViewportDimensions();
    return QObject::eventFilter(obj, ev);
}

void RegisterTableController::applyViewportDimensions()
{
    if (!m_view || !m_model) return;

    const int rowHeight = m_view->verticalHeader()->defaultSectionSize();
    if (kCellWidth <= 0 || rowHeight <= 0) return;

    const QSize vp = m_view->viewport()->size();

    int cols = vp.width() / kCellWidth;
    cols -= cols % 2;
    cols = std::max(2, cols);
    const int rows = std::max(1, vp.height() / rowHeight);

    if (m_model->isEditing())
        return;

    m_model->setGridDimensions(rows, cols);
}

void RegisterTableController::setWatches(const QVector<DeviceAddress>& items,
                                         const QVector<int>& segmentSizes)
{
    if (m_model) m_model->setWatches(items, segmentSizes);
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
