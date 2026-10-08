/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERTABLEMODEL_H
#define REGISTERTABLEMODEL_H

#include <QAbstractTableModel>
#include <QTimer>
#include <QElapsedTimer>
#include <QColor>
#include <QPersistentModelIndex>
#include <QVector>
#include <vector>

#include "Core/DeviceAddress.h"
#include "RegisterTableTypes.h"

class RegisterStore;

// 寄存器表数据模型:承载数据访问、校验/写回、显示格式化与闪烁状态。
// 文本按需经 data() 计算;store 变更只对变化格 emit dataChanged(定向刷新);
// 编辑保护 = 对正在编辑的 index 不发 dataChanged(真实 index 由委托上报)。
class RegisterTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RegisterTableModel(RegisterStore* store, QWidget* dialogParent, QObject* parent = nullptr);

    void setWatches(const QVector<DeviceAddress>& items,
                    const QVector<int>& segmentSizes = QVector<int>());
    void setGridDimensions(int rowCount, int colCount);
    void setDataType(RegisterDataType type);
    void setNumberBase(bool hex);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QColor flashOverlay(int row, int col) const;

    void setEditingIndex(const QModelIndex& index);
    void clearEditingIndex();
    bool isEditing() const { return m_editIndex.isValid(); }

    void refreshHeaders();

signals:
    void flashTick();
    void editingFinished();

private slots:
    void onStoreChanged();
    void onFlashTick();
    void flushStoreRefresh();

private:
    void syncDisplayCache();
    void refreshAll();
    QString formatValueAtRow(int row) const;
    QString formatDWord(RegisterDataType type, int wordIndex) const;
    bool writeDWord(RegisterDataType type, int wordIndex, const QString& text);
    int registersPerValue(RegisterDataType type) const;
    void stampFlash(int row, int col);
    int flashIndex(int row, int col) const;
    bool isValueEditable(int watchIndex) const;
    int watchIndexAt(int row, int col) const;
    QModelIndex valueIndexForWatch(int watchIndex) const;
    void rebuildWatchGrid();
    bool trySegmentedGridPlacement();

    RegisterStore* m_store;
    QWidget* m_dialogParent;
    RegisterCellLayout m_layout{ 21, 10 };

    QVector<DeviceAddress> m_items;
    QVector<int> m_segmentSizes;
    QVector<int> m_watchIndexGrid;  // row*colCount+col -> watch index, -1 empty
    bool m_segmentedLayout = false;
    QVector<QString> m_displayCache;

    RegisterDataType m_currentType;
    int m_intStat;
    std::vector<qint64> m_flashStartMs;
    QElapsedTimer m_clock;
    QTimer* m_flashTimer;
    QTimer* m_storeRefreshTimer;
    QColor m_flashColor;

    QPersistentModelIndex m_editIndex;
};

#endif // REGISTERTABLEMODEL_H
