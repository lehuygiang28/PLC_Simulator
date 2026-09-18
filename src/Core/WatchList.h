#ifndef CORE_WATCHLIST_H
#define CORE_WATCHLIST_H

#include "Core/DeviceAddress.h"
#include <QString>
#include <QVector>

namespace WatchList {
constexpr int kMaxItems = 2048;
bool parse(const QString& expr, QVector<DeviceAddress>& out, QString& error);
}

#endif
