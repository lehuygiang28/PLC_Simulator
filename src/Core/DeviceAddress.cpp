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

QString DeviceAddress::toString() const
{
    if (kind == DeviceKind::M)
        return QStringLiteral("M%1").arg(index);
    if (bit >= 0)
        return QStringLiteral("D%1.%2").arg(index).arg(bit);
    return QStringLiteral("D%1").arg(index);
}
