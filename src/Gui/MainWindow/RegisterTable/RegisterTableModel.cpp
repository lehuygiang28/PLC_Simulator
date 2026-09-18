/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "RegisterTableModel.h"
#include "Core/RegisterStore.h"
#include "Theme/ThemeManager.h"

#include <QMessageBox>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>
#include <cfloat>
#include <cstdint>

namespace
{
QString trRT(const char* text)
{
    return QCoreApplication::translate("RegisterTableModel", text);
}

constexpr int kFloatSigDigits = 7;
constexpr int kDoubleSigDigits = 15;

constexpr int kFlashDurationMs = 400;
constexpr int kFlashPeakAlpha  = 140;
constexpr int kFlashTickMs     = 30;

constexpr int kValueColumn = 1;

QString formatReal(double value, int sigDigits)
{
    return QString::number(value, 'g', sigDigits);
}

struct TypeTrait
{
    int registersPerValue;
    enum Family { Char, Integer, Real } family;
    long long intMin, intMax;
    int hexDigits;
    double realMin, realMax;
    int sigDigits;
};

const TypeTrait& traitOf(RegisterDataType type)
{
    static const TypeTrait kChar8 { 1, TypeTrait::Char,    0, 0, 0, 0.0, 0.0, 0 };
    static const TypeTrait kInt16 { 1, TypeTrait::Integer, INT16_MIN, INT16_MAX, 4, 0.0, 0.0, 0 };
    static const TypeTrait kInt32 { 2, TypeTrait::Integer, INT32_MIN, INT32_MAX, 8, 0.0, 0.0, 0 };
    static const TypeTrait kFloat { 2, TypeTrait::Real,    0, 0, 0, -FLT_MAX, FLT_MAX, kFloatSigDigits };
    static const TypeTrait kDouble{ 4, TypeTrait::Real,    0, 0, 0, -DBL_MAX, DBL_MAX, kDoubleSigDigits };
    switch (type)
    {
    case RegisterDataType::eDataTypeInt16:  return kInt16;
    case RegisterDataType::eDataTypeInt32:  return kInt32;
    case RegisterDataType::eDataTypeFloat:  return kFloat;
    case RegisterDataType::eDataTypeDouble: return kDouble;
    default:                                return kChar8;
    }
}

struct ValidationResult
{
    bool ok;
    QString normalized;
    QString title;
    QString msg;
};

ValidationResult validateChar(const QString& text)
{
    if (text.isEmpty())
        return { false, QString(), trRT("输入非法"),
                 trRT("输入为空,已保留原值!") };
    if (text.length() > 2)
        return { true, text.left(2), trRT("输入截断"),
                 trRT("输入值 %1 长度超过2,只保留前2位!").arg(text) };
    return { true, text, QString(), QString() };
}

ValidationResult validateInteger(const QString& text, const TypeTrait& t, int intStat)
{
    const QString s = text.trimmed();
    if (intStat == 1)
    {
        QRegularExpression hexRe("^[0-9A-Fa-f]+$");
        if (!hexRe.match(s).hasMatch())
            return { false, QString(), trRT("输入非法"),
                     trRT("输入值 %1 非十六进制数").arg(s) };
        if (s.length() > t.hexDigits)
            return { true, s.left(t.hexDigits).toUpper(), trRT("输入截断"),
                     trRT("输入值 %1 超过范围,将截断输入数据!").arg(s) };
        return { true, s.toUpper().rightJustified(t.hexDigits, '0'), QString(), QString() };
    }

    QRegularExpression decRe("^[+-]?\\d+$");
    if (!decRe.match(s).hasMatch())
        return { false, QString(), trRT("输入非法"),
                 trRT("输入值 %1 非整型数").arg(s) };
    bool ok = false;
    const long long v = s.toLongLong(&ok);
    if (!ok || v < t.intMin || v > t.intMax)
        return { false, QString(), trRT("输入超范围"),
                 trRT("输入值 %1 超过范围(%2 ~ %3)").arg(s).arg(t.intMin).arg(t.intMax) };
    return { true, QString::number(v), QString(), QString() };
}

ValidationResult validateReal(const QString& text, const TypeTrait& t)
{
    const QString s = text.trimmed();
    QRegularExpression re("^[+-]?(\\d+\\.?\\d*|\\.\\d+)$");
    if (!re.match(s).hasMatch())
        return { false, QString(), trRT("输入非法"),
                 trRT("输入值 %1 非浮点数").arg(s) };
    const double v = s.toDouble();
    if (v < t.realMin || v > t.realMax)
        return { false, QString(), trRT("输入超范围"),
                 trRT("输入值 %1 超过浮点数范围").arg(s) };
    return { true, formatReal(v, t.sigDigits), QString(), QString() };
}

ValidationResult validateInput(const QString& text, RegisterDataType type, int intStat)
{
    const TypeTrait& t = traitOf(type);
    switch (t.family)
    {
    case TypeTrait::Char:    return validateChar(text);
    case TypeTrait::Integer: return validateInteger(text, t, intStat);
    case TypeTrait::Real:    return validateReal(text, t);
    }
    return { false, QString(), QString(), QString() };
}

ValidationResult validateBit(const QString& text)
{
    const QString s = text.trimmed().toLower();
    if (s == QLatin1String("0") || s == QLatin1String("false"))
        return { true, QStringLiteral("0"), QString(), QString() };
    if (s == QLatin1String("1") || s == QLatin1String("true"))
        return { true, QStringLiteral("1"), QString(), QString() };
    return { false, QString(), trRT("输入非法"),
             trRT("位值须为 0 或 1") };
}
}  // namespace

RegisterTableModel::RegisterTableModel(RegisterStore* store, QWidget* dialogParent, QObject* parent)
    : QAbstractTableModel(parent)
    , m_store(store)
    , m_dialogParent(dialogParent)
    , m_currentType(RegisterDataType::eDataTypeInt16)
    , m_intStat(0)
    , m_flashTimer(nullptr)
{
    m_clock.start();

    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(kFlashTickMs);
    connect(m_flashTimer, &QTimer::timeout, this, &RegisterTableModel::onFlashTick);

    m_flashColor = ThemeManager::instance().color("@flashBg");
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]
    {
        m_flashColor = ThemeManager::instance().color("@flashBg");
        emit flashTick();
    });

    if (m_store)
        connect(m_store, &RegisterStore::dataChanged, this, &RegisterTableModel::onStoreChanged,
                Qt::UniqueConnection);
}

void RegisterTableModel::setWatches(const QVector<DeviceAddress>& items)
{
    beginResetModel();
    m_items = items;
    m_flashStartMs.assign(m_items.size() * columnCount(), 0);
    if (m_flashTimer->isActive())
        m_flashTimer->stop();
    m_editIndex = QPersistentModelIndex();
    syncDisplayCache();
    endResetModel();
}

int RegisterTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

int RegisterTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant RegisterTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignCenter);
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const int row = index.row();
    if (row < 0 || row >= m_items.size())
        return QVariant();

    if (index.column() == 0)
        return m_items[row].toString();

    if (!isValueEditable(row))
        return QString();
    return formatValueAtRow(row);
}

QVariant RegisterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Horizontal)
        return section == kValueColumn ? tr("值") : tr("地址");
    return QStringLiteral(" ");
}

Qt::ItemFlags RegisterTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == kValueColumn && isValueEditable(index.row()))
        f |= Qt::ItemIsEditable;
    return f;
}

bool RegisterTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role != Qt::EditRole || !index.isValid() || !m_store) return false;
    if (index.column() != kValueColumn) return false;

    const int row = index.row();
    if (row < 0 || row >= m_items.size() || !isValueEditable(row))
        return false;

    const DeviceAddress& addr = m_items[row];
    ValidationResult r;
    if (addr.isBit())
        r = validateBit(value.toString());
    else
        r = validateInput(value.toString(), m_currentType, m_intStat);

    if (!r.title.isEmpty())
    {
        const QString title = r.title;
        const QString msg = r.msg;
        QTimer::singleShot(0, this, [this, title, msg]
        {
            QMessageBox::warning(m_dialogParent, title, msg);
        });
    }

    if (!r.ok) return false;

    if (addr.isBit())
    {
        m_store->SetBit(addr, r.normalized == QLatin1String("1"));
    }
    else
    {
        if (!writeDWord(m_currentType, addr.index, r.normalized))
            return false;
    }

    if (row < m_displayCache.size())
        m_displayCache[row] = formatValueAtRow(row);

    stampFlash(row, kValueColumn);
    emit dataChanged(index, index);
    return true;
}

void RegisterTableModel::setDataType(RegisterDataType type)
{
    m_currentType = type;
    refreshAll();
}

void RegisterTableModel::setNumberBase(bool hex)
{
    m_intStat = hex ? 1 : 0;
    refreshAll();
}

QColor RegisterTableModel::flashOverlay(int row, int col) const
{
    const int idx = flashIndex(row, col);
    if (idx < 0) return QColor();
    const qint64 start = m_flashStartMs[idx];
    if (start == 0) return QColor();
    const qint64 elapsed = m_clock.elapsed() - start;
    if (elapsed < 0 || elapsed >= kFlashDurationMs) return QColor();

    QColor c = m_flashColor;
    c.setAlpha(static_cast<int>(kFlashPeakAlpha * (kFlashDurationMs - elapsed) / kFlashDurationMs));
    return c;
}

void RegisterTableModel::stampFlash(int row, int col)
{
    const int idx = flashIndex(row, col);
    if (idx < 0) return;
    m_flashStartMs[idx] = m_clock.elapsed();
    if (!m_flashTimer->isActive()) m_flashTimer->start();
}

int RegisterTableModel::flashIndex(int row, int col) const
{
    const int cols = columnCount();
    if (cols <= 0) return -1;
    const int idx = row * cols + col;
    if (idx < 0 || idx >= static_cast<int>(m_flashStartMs.size())) return -1;
    return idx;
}

void RegisterTableModel::setEditingIndex(const QModelIndex& index)
{
    m_editIndex = index;
}

void RegisterTableModel::clearEditingIndex()
{
    const QModelIndex idx = m_editIndex;
    m_editIndex = QPersistentModelIndex();
    if (idx.isValid())
        emit dataChanged(idx, idx);
    emit editingFinished();
}

void RegisterTableModel::onStoreChanged()
{
    if (m_items.isEmpty()) return;

    const int rows = m_items.size();
    if (m_displayCache.size() != rows)
        m_displayCache.resize(rows);

    for (int row = 0; row < rows; ++row)
    {
        if (!isValueEditable(row)) continue;

        const QString newText = formatValueAtRow(row);
        if (m_displayCache[row] == newText) continue;
        m_displayCache[row] = newText;

        const QModelIndex idx = index(row, kValueColumn);
        if (m_editIndex == idx) continue;
        stampFlash(row, kValueColumn);
        emit dataChanged(idx, idx);
    }
}

void RegisterTableModel::onFlashTick()
{
    emit flashTick();

    const qint64 now = m_clock.elapsed();
    bool anyActive = false;
    for (qint64& start : m_flashStartMs)
    {
        if (start == 0) continue;
        if (now - start >= kFlashDurationMs) start = 0;
        else anyActive = true;
    }
    if (!anyActive) m_flashTimer->stop();
}

void RegisterTableModel::syncDisplayCache()
{
    m_displayCache.resize(m_items.size());
    for (int row = 0; row < m_items.size(); ++row)
        m_displayCache[row] = formatValueAtRow(row);
}

void RegisterTableModel::refreshAll()
{
    syncDisplayCache();
    if (!m_items.isEmpty())
        emit dataChanged(index(0, 0), index(m_items.size() - 1, columnCount() - 1));
}

int RegisterTableModel::registersPerValue(RegisterDataType type) const
{
    return traitOf(type).registersPerValue;
}

bool RegisterTableModel::isValueEditable(int row) const
{
    if (row < 0 || row >= m_items.size() || !m_store) return false;
    const DeviceAddress& addr = m_items[row];
    if (addr.isBit()) return true;
    if (addr.kind != DeviceKind::D || addr.bit >= 0) return false;
    const int rpv = registersPerValue(m_currentType);
    return addr.index + rpv - 1 < m_store->size();
}

QString RegisterTableModel::formatValueAtRow(int row) const
{
    if (!m_store || row < 0 || row >= m_items.size()) return QString();
    const DeviceAddress& addr = m_items[row];
    if (addr.isBit())
        return m_store->GetBit(addr) ? QStringLiteral("1") : QStringLiteral("0");
    return formatDWord(m_currentType, addr.index);
}

QString RegisterTableModel::formatDWord(RegisterDataType type, int wordIndex) const
{
    if (!m_store) return QString();
    const TypeTrait& t = traitOf(type);
    switch (t.family)
    {
    case TypeTrait::Char:
        return m_store->GetString(wordIndex);
    case TypeTrait::Integer:
    {
        if (m_intStat == 1)
        {
            const unsigned long long bits = (t.registersPerValue == 1)
                ? static_cast<unsigned long long>(static_cast<uint16_t>(m_store->GetInt16(wordIndex)))
                : static_cast<unsigned long long>(static_cast<uint32_t>(m_store->GetInt32(wordIndex)));
            return QString("%1").arg(QString::number(bits, 16), t.hexDigits, QChar('0')).toUpper();
        }
        const long long v = (t.registersPerValue == 1)
            ? m_store->GetInt16(wordIndex)
            : m_store->GetInt32(wordIndex);
        return QString::number(v);
    }
    case TypeTrait::Real:
    {
        const double v = (t.registersPerValue == 2)
            ? static_cast<double>(m_store->GetFloat(wordIndex))
            : m_store->GetDouble(wordIndex);
        return formatReal(v, t.sigDigits);
    }
    }
    return QString();
}

bool RegisterTableModel::writeDWord(RegisterDataType type, int wordIndex, const QString& text)
{
    if (!m_store) return false;
    const TypeTrait& t = traitOf(type);
    switch (t.family)
    {
    case TypeTrait::Char:
        m_store->SetString(wordIndex, text);
        return true;
    case TypeTrait::Integer:
    {
        int val;
        if (m_intStat == 0)
            val = text.toInt();
        else
            val = static_cast<int>(text.toUInt(nullptr, 16));
        if (t.registersPerValue == 1)
            m_store->SetInt16(wordIndex, static_cast<int16_t>(val & 0xFFFF));
        else
            m_store->SetInt32(wordIndex, val);
        return true;
    }
    case TypeTrait::Real:
        if (t.registersPerValue == 2)
            m_store->SetFloat(wordIndex, text.toFloat());
        else
            m_store->SetDouble(wordIndex, text.toDouble());
        return true;
    }
    return false;
}

void RegisterTableModel::refreshHeaders()
{
    if (columnCount() > 0)
        emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}
