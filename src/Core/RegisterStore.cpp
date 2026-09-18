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
    : QObject(parent)
    , m_cells(kRegisterCount)
    , m_mWords(mWordCount())
{
    for (auto& c : m_cells) c.store(0, std::memory_order_relaxed);
    for (auto& c : m_mWords) c.store(0, std::memory_order_relaxed);
}

bool RegisterStore::mBit(int index) const
{
    if (index < 0 || index >= kBitCount) return false;
    const int w = index / 16;
    const int b = index % 16;
    return (m_mWords[w].load() & (int16_t(1) << b)) != 0;
}

bool RegisterStore::setMBit(int index, bool value)
{
    if (index < 0 || index >= kBitCount) return false;
    const int w = index / 16;
    const int b = index % 16;
    const int16_t mask = static_cast<int16_t>(1 << b);
    const int16_t prev = m_mWords[w].load();
    const int16_t next = value ? static_cast<int16_t>(prev | mask)
                               : static_cast<int16_t>(prev & static_cast<int16_t>(~mask));
    m_mWords[w].store(next);
    return prev != next;
}

bool RegisterStore::dBit(int word, int bit) const
{
    if (word < 0 || word >= kRegisterCount || bit < 0 || bit > 15) return false;
    return (cell(word) & (int16_t(1) << bit)) != 0;
}

bool RegisterStore::setDBit(int word, int bit, bool value)
{
    if (word < 0 || word >= kRegisterCount || bit < 0 || bit > 15) return false;
    const int16_t mask = static_cast<int16_t>(1 << bit);
    const int16_t prev = cell(word);
    const int16_t next = value ? static_cast<int16_t>(prev | mask)
                               : static_cast<int16_t>(prev & static_cast<int16_t>(~mask));
    return setCell(word, next);
}

bool RegisterStore::GetBit(const DeviceAddress& addr) const
{
    if (addr.kind == DeviceKind::M) return mBit(addr.index);
    if (addr.bit >= 0) return dBit(addr.index, addr.bit);
    return false;
}

bool RegisterStore::SetBit(const DeviceAddress& addr, bool value)
{
    bool changed = false;
    if (addr.kind == DeviceKind::M) changed = setMBit(addr.index, value);
    else if (addr.bit >= 0) changed = setDBit(addr.index, addr.bit, value);
    emit dataChanged();
    return changed;
}

std::vector<uint8_t> RegisterStore::bits(DeviceKind kind, int start, int count) const
{
    std::vector<uint8_t> out;
    if (count <= 0) return out;
    out.resize(count, 0);
    for (int i = 0; i < count; ++i) {
        if (kind == DeviceKind::M) {
            out[i] = mBit(start + i) ? 1 : 0;
        } else {
            const int word = start + i / 16;
            const int b = i % 16;
            out[i] = dBit(word, b) ? 1 : 0;
        }
    }
    return out;
}

bool RegisterStore::setBits(DeviceKind kind, int start, const std::vector<uint8_t>& values)
{
    bool changed = false;
    for (size_t i = 0; i < values.size(); ++i) {
        const bool on = values[i] != 0;
        if (kind == DeviceKind::M) {
            if (setMBit(start + static_cast<int>(i), on)) changed = true;
        } else {
            const int absBit = static_cast<int>(i);
            if (setDBit(start + absBit / 16, absBit % 16, on)) changed = true;
        }
    }
    return changed;
}

std::vector<int16_t> RegisterStore::words(DeviceKind kind, int start, int count) const
{
    if (kind == DeviceKind::D) return cells(start, count);
    std::vector<int16_t> out;
    if (count <= 0 || start % 16 != 0) return out;
    out.reserve(count);
    for (int w = 0; w < count; ++w) {
        const int bitStart = start + w * 16;
        int16_t packed = 0;
        for (int b = 0; b < 16; ++b) {
            if (mBit(bitStart + b)) packed = static_cast<int16_t>(packed | (1 << b));
        }
        out.push_back(packed);
    }
    return out;
}

bool RegisterStore::setWords(DeviceKind kind, int start, const std::vector<int16_t>& values)
{
    if (kind == DeviceKind::D) return setCells(start, values);
    if (start % 16 != 0) return false;
    bool changed = false;
    for (size_t w = 0; w < values.size(); ++w) {
        const int bitStart = start + static_cast<int>(w) * 16;
        for (int b = 0; b < 16; ++b) {
            const bool on = (values[w] & (1 << b)) != 0;
            if (setMBit(bitStart + b, on)) changed = true;
        }
    }
    return changed;
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
    for (auto& c : m_mWords) c.store(0);
    emit dataChanged();
    return true;
}

void RegisterStore::notifyChanged()
{
    emit dataChanged();
}
