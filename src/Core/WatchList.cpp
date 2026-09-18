#include "Core/WatchList.h"

static bool parseRangeEnd(const DeviceAddress& start, const QString& endToken, DeviceAddress& end, QString& error)
{
    if (DeviceAddress::parse(endToken, end)) {
        if (end.kind != start.kind) {
            error = QStringLiteral("Range devices must match: %1-%2").arg(start.toString(), end.toString());
            return false;
        }
        return true;
    }
    bool ok = false;
    const int idx = endToken.toInt(&ok);
    if (!ok || idx < 0 || idx > DeviceAddress::kMaxIndex) {
        error = QStringLiteral("Invalid range end: %1").arg(endToken);
        return false;
    }
    end = start;
    end.index = idx;
    return true;
}

static bool appendToken(const QString& token, QVector<DeviceAddress>& out, int& segmentSize, QString& error)
{
    segmentSize = 0;
    const int dash = token.indexOf(QLatin1Char('-'), 1);
    if (dash < 0) {
        DeviceAddress addr;
        if (!DeviceAddress::parse(token, addr)) {
            error = QStringLiteral("Invalid address: %1").arg(token);
            return false;
        }
        if (out.size() >= WatchList::kMaxItems) {
            error = QStringLiteral("Watch list exceeds %1 items").arg(WatchList::kMaxItems);
            return false;
        }
        out.push_back(addr);
        segmentSize = 1;
        return true;
    }

    const QString left = token.left(dash).trimmed();
    const QString right = token.mid(dash + 1).trimmed();
    DeviceAddress start;
    if (!DeviceAddress::parse(left, start)) {
        error = QStringLiteral("Invalid address: %1").arg(left);
        return false;
    }
    if (start.bit >= 0) {
        error = QStringLiteral("Bit ranges are not supported: %1").arg(token);
        return false;
    }
    DeviceAddress end;
    if (!parseRangeEnd(start, right, end, error))
        return false;
    if (end.bit >= 0) {
        error = QStringLiteral("Bit ranges are not supported: %1").arg(token);
        return false;
    }
    if (end.index < start.index) {
        error = QStringLiteral("Inverted range: %1").arg(token);
        return false;
    }
    const int count = end.index - start.index + 1;
    if (out.size() + count > WatchList::kMaxItems) {
        error = QStringLiteral("Watch list exceeds %1 items").arg(WatchList::kMaxItems);
        return false;
    }
    for (int i = start.index; i <= end.index; ++i) {
        DeviceAddress item = start;
        item.index = i;
        out.push_back(item);
    }
    segmentSize = count;
    return true;
}

bool WatchList::parseWithSegments(const QString& expr, ParseResult& out, QString& error)
{
    out.items.clear();
    out.segmentSizes.clear();
    error.clear();

    const QStringList tokens = expr.split(QLatin1Char(','), Qt::KeepEmptyParts);
    for (const QString& raw : tokens) {
        const QString token = raw.trimmed();
        if (token.isEmpty()) continue;

        int segmentSize = 0;
        if (!appendToken(token, out.items, segmentSize, error)) {
            out.items.clear();
            out.segmentSizes.clear();
            return false;
        }
        out.segmentSizes.push_back(segmentSize);
    }

    if (out.items.isEmpty()) {
        error = QStringLiteral("Watch list is empty");
        return false;
    }
    return true;
}

bool WatchList::parse(const QString& expr, QVector<DeviceAddress>& out, QString& error)
{
    ParseResult result;
    if (!parseWithSegments(expr, result, error))
        return false;
    out = result.items;
    return true;
}
