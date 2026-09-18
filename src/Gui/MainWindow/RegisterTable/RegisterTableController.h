/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERTABLECONTROLLER_H
#define REGISTERTABLECONTROLLER_H

#include <QObject>
#include <QVector>

#include "Core/DeviceAddress.h"
#include "RegisterTableTypes.h"

class QTableView;
class RegisterStore;
class RegisterTableModel;

class RegisterTableController : public QObject
{
    Q_OBJECT

public:
    explicit RegisterTableController(QTableView* view, RegisterStore* store, QWidget* parent = nullptr);

    ~RegisterTableController() override = default;

    void initTable();

    void setWatches(const QVector<DeviceAddress>& items,
                    const QVector<int>& segmentSizes = QVector<int>());
    void setDataType(RegisterDataType type);
    void setNumberBase(bool hex);
    void retranslateHeaders();

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void applyViewportDimensions();

    static constexpr int kCellWidth = 80;

    QTableView* m_view;
    RegisterStore* m_store;
    QWidget* m_parentWidget;
    RegisterTableModel* m_model;
};

#endif // REGISTERTABLECONTROLLER_H
