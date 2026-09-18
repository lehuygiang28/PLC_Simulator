#include "tst_RegisterTableModel.h"
#include "Core/WatchList.h"
#include "Core/RegisterStore.h"
#include "RegisterTableModel.h"

#include <QtTest>

void tst_RegisterTableModel::rows_follow_watch_items()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    QVector<DeviceAddress> items;
    QString err;
    QVERIFY(WatchList::parse(QStringLiteral("M1500-1501, D7"), items, err));
    model.setGridDimensions(10, 4);
    model.setWatches(items, QVector<int>({ 2, 1 }));
    QCOMPARE(model.rowCount(), 10);
    QCOMPARE(model.columnCount(), 4);
    QCOMPARE(model.data(model.index(0, 0)).toString(), QStringLiteral("M1500"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QStringLiteral("D7"));
}

void tst_RegisterTableModel::segmented_layout_starts_each_comma_token_in_new_column_pair()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    WatchList::ParseResult parsed;
    QString err;
    QVERIFY(WatchList::parseWithSegments(QStringLiteral("M10, M20, D30"), parsed, err));
    model.setGridDimensions(5, 8);
    model.setWatches(parsed.items, parsed.segmentSizes);
    QCOMPARE(model.data(model.index(0, 0)).toString(), QStringLiteral("M10"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QStringLiteral("M20"));
    QCOMPARE(model.data(model.index(0, 4)).toString(), QStringLiteral("D30"));
}

void tst_RegisterTableModel::edit_m_bit()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("M3"), a);
    model.setGridDimensions(5, 2);
    model.setWatches({a});
    QVERIFY(model.setData(model.index(0, 1), QStringLiteral("1"), Qt::EditRole));
    QVERIFY(store.GetBit(a));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QStringLiteral("1"));
}

void tst_RegisterTableModel::m_and_d_use_distinct_watch_colors()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    DeviceAddress mAddr;
    DeviceAddress dAddr;
    QVERIFY(DeviceAddress::parse(QStringLiteral("M3"), mAddr));
    QVERIFY(DeviceAddress::parse(QStringLiteral("D5"), dAddr));
    model.setGridDimensions(5, 2);
    model.setWatches({ mAddr, dAddr });

    const QColor mAddrFg = model.data(model.index(0, 0), Qt::ForegroundRole).value<QColor>();
    const QColor dAddrFg = model.data(model.index(1, 0), Qt::ForegroundRole).value<QColor>();
    const QColor mAddrBg = model.data(model.index(0, 0), Qt::BackgroundRole).value<QColor>();
    const QColor dAddrBg = model.data(model.index(1, 0), Qt::BackgroundRole).value<QColor>();
    QVERIFY(mAddrFg.isValid() && dAddrFg.isValid());
    QVERIFY(mAddrFg != dAddrFg);
    QVERIFY(mAddrBg.isValid() && dAddrBg.isValid());
    QVERIFY(mAddrBg != dAddrBg);
}

void tst_RegisterTableModel::edit_d_word_int16()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("D5"), a);
    model.setGridDimensions(5, 2);
    model.setWatches({a});
    model.setDataType(RegisterDataType::eDataTypeInt16);
    QVERIFY(model.setData(model.index(0, 1), QStringLiteral("42"), Qt::EditRole));
    QCOMPARE(store.GetInt16(5), int16_t(42));
}
