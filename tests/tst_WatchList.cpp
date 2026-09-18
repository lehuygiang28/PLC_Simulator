#include "tst_WatchList.h"
#include "Core/WatchList.h"

#include <QtTest>

void tst_WatchList::parse_mixed_ranges_and_singles()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(WatchList::parse(QStringLiteral("M1500-1559, M1564,M1565,D2024"), items, error));
    QCOMPARE(error, QString());
    QCOMPARE(items.size(), 60 + 2 + 1);
    QCOMPARE(items.front().toString(), QStringLiteral("M1500"));
    QCOMPARE(items[59].toString(), QStringLiteral("M1559"));
    QCOMPARE(items[60].toString(), QStringLiteral("M1564"));
    QCOMPARE(items[61].toString(), QStringLiteral("M1565"));
    QCOMPARE(items.back().toString(), QStringLiteral("D2024"));
}

void tst_WatchList::parse_allows_repeated_prefix_on_range_end()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(WatchList::parse(QStringLiteral("D10-D12"), items, error));
    QCOMPARE(items.size(), 3);
    QCOMPARE(items[1].toString(), QStringLiteral("D11"));
}

void tst_WatchList::parse_skips_empty_tokens()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(WatchList::parse(QStringLiteral("M1,, M2,"), items, error));
    QCOMPARE(items.size(), 2);
}

void tst_WatchList::parse_rejects_cross_device_range()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(!WatchList::parse(QStringLiteral("M1500-D1559"), items, error));
    QVERIFY(!error.isEmpty());
}

void tst_WatchList::parse_rejects_d_bit_range()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(!WatchList::parse(QStringLiteral("D2024.0-3"), items, error));
}

void tst_WatchList::parse_rejects_inverted_range()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(!WatchList::parse(QStringLiteral("M10-5"), items, error));
}

void tst_WatchList::parse_rejects_too_many_items()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(!WatchList::parse(QStringLiteral("M0-2048"), items, error));
    QVERIFY(WatchList::parse(QStringLiteral("M0-2047"), items, error));
    QCOMPARE(items.size(), 2048);
}

void tst_WatchList::parse_rejects_empty_expression()
{
    QVector<DeviceAddress> items;
    QString error;
    QVERIFY(!WatchList::parse(QStringLiteral("  , ,"), items, error));
}

void tst_WatchList::parseWithSegments_records_comma_boundaries()
{
    WatchList::ParseResult result;
    QString error;
    QVERIFY(WatchList::parseWithSegments(QStringLiteral("M1500-1501, D7"), result, error));
    QCOMPARE(result.items.size(), 3);
    QCOMPARE(result.segmentSizes.size(), 2);
    QCOMPARE(result.segmentSizes[0], 2);
    QCOMPARE(result.segmentSizes[1], 1);
}
