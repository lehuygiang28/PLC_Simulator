#include "Core/DeviceAddress.h"

#include <QChar>

static bool parseIndex(const QString& digits, int& out)
{
    if (digits.isEmpty()) return false;
    for (const QChar ch : digits) {
        if (!ch.isDigit()) return false;
    }
    bool ok = false;
    const int v = digits.toInt(&ok);
    if (!ok || v < 0 || v > DeviceAddress::kMaxIndex) return false;
    out = v;
    return true;
}

bool DeviceAddress::parse(const QString& text, DeviceAddress& out)
{
    const QString s = text.trimmed();
    if (s.size() < 2) return false;

    const QChar prefix = s[0].toUpper();
    DeviceKind kind;
    if (prefix == QLatin1Char('D')) kind = DeviceKind::D;
    else if (prefix == QLatin1Char('M')) kind = DeviceKind::M;
    else return false;

    const QString rest = s.mid(1);
    DeviceAddress parsed;
    parsed.kind = kind;
    parsed.bit = -1;

    if (kind == DeviceKind::M) {
        if (!parseIndex(rest, parsed.index)) return false;
        out = parsed;
        return true;
    }

    const int dot = rest.indexOf(QLatin1Char('.'));
    if (dot < 0) {
        if (!parseIndex(rest, parsed.index)) return false;
        out = parsed;
        return true;
    }

    if (!parseIndex(rest.left(dot), parsed.index)) return false;
    int bit = 0;
    if (!parseIndex(rest.mid(dot + 1), bit)) return false;
    if (bit < 0 || bit > 15) return false;
    parsed.bit = bit;
    out = parsed;
    return true;
}

bool parseValueView(const QString& type, const DeviceAddress& addr, ValueView& out, QString& error)
{
    error.clear();
    const QString t = type.trimmed().toLower();
    if (t.isEmpty() || t == QLatin1String("auto")) {
        out = addr.isBit() ? ValueView::Bit : ValueView::Int16;
        return true;
    }
    if (t == QLatin1String("bit")) {
        if (!addr.isBit()) {
            error = QStringLiteral("type=bit requires M or D.n");
            return false;
        }
        out = ValueView::Bit;
        return true;
    }
    if (addr.isBit()) {
        error = QStringLiteral("bit address requires type=bit");
        return false;
    }
    if (t == QLatin1String("int16") || t == QLatin1String("int")) {
        out = ValueView::Int16;
        return true;
    }
    if (t == QLatin1String("int32") || t == QLatin1String("dword")) {
        out = ValueView::Int32;
        return true;
    }
    if (t == QLatin1String("float")) {
        out = ValueView::Float;
        return true;
    }
    if (t == QLatin1String("double")) {
        out = ValueView::Double;
        return true;
    }
    if (t == QLatin1String("string")) {
        out = ValueView::String;
        return true;
    }
    error = QStringLiteral("Unsupported register type: %1").arg(type);
    return false;
}

QString DeviceAddress::toString() const
{
    if (kind == DeviceKind::M)
        return QStringLiteral("M%1").arg(index);
    if (bit >= 0)
        return QStringLiteral("D%1.%2").arg(index).arg(bit);
    return QStringLiteral("D%1").arg(index);
}
