#ifndef CORE_WATCHLIST_H
#define CORE_WATCHLIST_H

#include "Core/DeviceAddress.h"
#include <QString>
#include <QVector>

namespace WatchList {
constexpr int kMaxItems = 2048;

struct ParseResult
{
    QVector<DeviceAddress> items;
    QVector<int> segmentSizes;  // each comma-separated token; sums to items.size()
};

bool parse(const QString& expr, QVector<DeviceAddress>& out, QString& error);
bool parseWithSegments(const QString& expr, ParseResult& out, QString& error);
}

#endif
