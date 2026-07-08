/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "Core/RegisterStore.h"
#include "Core/DataTypeConvert.h"

RegisterStore::RegisterStore(QObject* parent)
    : QObject(parent), m_cells(kRegisterCount)
{
    for (auto& c : m_cells) c.store(0, std::memory_order_relaxed);
}

int16_t RegisterStore::cell(int index) const
{
    if (index < 0 || index >= kRegisterCount) return 0;
    return m_cells[index].load();
}

bool RegisterStore::setCell(int index, int16_t value)
{
    if (index < 0 || index >= kRegisterCount) return false;
    const int16_t prev = m_cells[index].load();
    m_cells[index].store(value);
    return prev != value;
}

std::vector<int16_t> RegisterStore::cells(int start, int count) const
{
    std::vector<int16_t> out;
    if (count <= 0) return out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int idx = start + i;
        out.push_back((idx >= 0 && idx < kRegisterCount) ? m_cells[idx].load() : int16_t(0));
    }
    return out;
}

bool RegisterStore::setCells(int start, const std::vector<int16_t>& values)
{
    bool changed = false;
    for (size_t i = 0; i < values.size(); ++i) {
        const int idx = start + static_cast<int>(i);
        if (idx < 0 || idx >= kRegisterCount) break;
        if (setCell(idx, values[i])) changed = true;
    }
    return changed;
}

int16_t RegisterStore::GetInt16(int index) const { return cell(index); }

int32_t RegisterStore::GetInt32(int index) const
{
    DataTypeConvert dt;
    dt.u_Int16[0] = cell(index);
    dt.u_Int16[1] = cell(index + 1);
    return dt.u_Int32[0];
}

float RegisterStore::GetFloat(int index) const
{
    DataTypeConvert dt;
    dt.u_Int16[0] = cell(index);
    dt.u_Int16[1] = cell(index + 1);
    return dt.u_float[0];
}

double RegisterStore::GetDouble(int index) const
{
    DataTypeConvert dt;
    dt.u_Int16[0] = cell(index);
    dt.u_Int16[1] = cell(index + 1);
    dt.u_Int16[2] = cell(index + 2);
    dt.u_Int16[3] = cell(index + 3);
    return dt.u_double;
}

QString RegisterStore::GetString(int index) const
{
    DataTypeConvert dt;
    dt.u_Int16[0] = cell(index);
    return QString("%1%2").arg(QChar(dt.u_chars[0])).arg(QChar(dt.u_chars[1]));
}

void RegisterStore::SetInt16(int index, int16_t value)
{
    setCell(index, value);
    emit dataChanged();
}

void RegisterStore::SetInt32(int index, int32_t value)
{
    DataTypeConvert dt;
    dt.u_Int32[0] = value;
    setCells(index, {dt.u_Int16[0], dt.u_Int16[1]});
    emit dataChanged();
}

void RegisterStore::SetFloat(int index, float value)
{
    DataTypeConvert dt;
    dt.u_float[0] = value;
    setCells(index, {dt.u_Int16[0], dt.u_Int16[1]});
    emit dataChanged();
}

void RegisterStore::SetDouble(int index, double value)
{
    DataTypeConvert dt;
    dt.u_double = value;
    setCells(index, {dt.u_Int16[0], dt.u_Int16[1], dt.u_Int16[2], dt.u_Int16[3]});
    emit dataChanged();
}

void RegisterStore::SetString(int index, const QString& value)
{
    DataTypeConvert dt;
    for (int i = 0; i < value.length() && i < 2; ++i)
        dt.u_chars[i] = value[i].toLatin1();
    setCell(index, dt.u_Int16[0]);
    emit dataChanged();
}

bool RegisterStore::resetAll(int16_t value)
{
    for (auto& c : m_cells) c.store(value);
    emit dataChanged();
    return true;
}

void RegisterStore::notifyChanged()
{
    emit dataChanged();
}
