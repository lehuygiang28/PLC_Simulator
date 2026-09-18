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
    model.setWatches(items);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 2);
    QCOMPARE(model.data(model.index(0, 0)).toString(), QStringLiteral("M1500"));
    QCOMPARE(model.data(model.index(2, 0)).toString(), QStringLiteral("D7"));
}

void tst_RegisterTableModel::edit_m_bit()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("M3"), a);
    model.setWatches({a});
    QVERIFY(model.setData(model.index(0, 1), QStringLiteral("1"), Qt::EditRole));
    QVERIFY(store.GetBit(a));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QStringLiteral("1"));
}

void tst_RegisterTableModel::edit_d_word_int16()
{
    RegisterStore store;
    RegisterTableModel model(&store, nullptr);
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("D5"), a);
    model.setWatches({a});
    model.setDataType(RegisterDataType::eDataTypeInt16);
    QVERIFY(model.setData(model.index(0, 1), QStringLiteral("42"), Qt::EditRole));
    QCOMPARE(store.GetInt16(5), int16_t(42));
}
