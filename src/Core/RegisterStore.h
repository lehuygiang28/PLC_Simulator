/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef CORE_REGISTERSTORE_H
#define CORE_REGISTERSTORE_H

#include <QObject>
#include <QString>
#include <vector>
#include <atomic>
#include <cstdint>

// 寄存器数据模型:唯一持有 int16 单元数组,提供裸/区间/类型化三档访问。
// 裸单元与裸区间静默;类型化 Set* 每次 emit dataChanged();批量走 notifyChanged()。
class RegisterStore : public QObject
{
    Q_OBJECT
public:
    static constexpr int kRegisterCount = 100000;

    explicit RegisterStore(QObject* parent = nullptr);

    int size() const { return kRegisterCount; }

    // 裸单元(静默)
    int16_t cell(int index) const;
    bool    setCell(int index, int16_t value);

    // 裸区间(静默);setCells 返回是否有任一单元变化
    std::vector<int16_t> cells(int start, int count) const;
    bool setCells(int start, const std::vector<int16_t>& values);

    // 类型化(每次 Set 后 emit dataChanged())
    int16_t GetInt16(int index) const;
    int32_t GetInt32(int index) const;
    float   GetFloat(int index) const;
    double  GetDouble(int index) const;
    QString GetString(int index) const;

    void SetInt16(int index, int16_t value);
    void SetInt32(int index, int32_t value);
    void SetFloat(int index, float value);
    void SetDouble(int index, double value);
    void SetString(int index, const QString& value);

    bool resetAll(int16_t value);   // emit 一次
    void notifyChanged();           // emit 一次

signals:
    void dataChanged();

private:
    std::vector<std::atomic_int16_t> m_cells;
};

#endif // CORE_REGISTERSTORE_H
