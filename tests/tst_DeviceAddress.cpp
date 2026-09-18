#include "tst_DeviceAddress.h"
#include "Core/DeviceAddress.h"

#include <QtTest>

void tst_DeviceAddress::parse_accepts_d_word()
{
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("D2024"), a));
    QCOMPARE(a.kind, DeviceKind::D);
    QCOMPARE(a.index, 2024);
    QCOMPARE(a.bit, -1);
    QVERIFY(!a.isBit());
}

void tst_DeviceAddress::parse_accepts_m_bit()
{
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("M1500"), a));
    QCOMPARE(a.kind, DeviceKind::M);
    QCOMPARE(a.index, 1500);
    QCOMPARE(a.bit, -1);
    QVERIFY(a.isBit());
}

void tst_DeviceAddress::parse_accepts_d_bit()
{
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("D2024.3"), a));
    QCOMPARE(a.kind, DeviceKind::D);
    QCOMPARE(a.index, 2024);
    QCOMPARE(a.bit, 3);
    QVERIFY(a.isBit());
}

void tst_DeviceAddress::parse_is_case_insensitive()
{
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("m1564"), a));
    QCOMPARE(a.kind, DeviceKind::M);
    QCOMPARE(a.index, 1564);
}

void tst_DeviceAddress::parse_rejects_unknown_device()
{
    DeviceAddress a;
    QVERIFY(!DeviceAddress::parse(QStringLiteral("X0"), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral("100"), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral(""), a));
}

void tst_DeviceAddress::parse_rejects_out_of_range()
{
    DeviceAddress a;
    QVERIFY(!DeviceAddress::parse(QStringLiteral("D100000"), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral("M-1"), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral("D"), a));
}

void tst_DeviceAddress::parse_rejects_bad_d_bit()
{
    DeviceAddress a;
    QVERIFY(!DeviceAddress::parse(QStringLiteral("D2024.16"), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral("D2024."), a));
    QVERIFY(!DeviceAddress::parse(QStringLiteral("M1500.0"), a));
}

void tst_DeviceAddress::value_view_defaults_bit_for_m()
{
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("M1"), a);
    ValueView v;
    QString err;
    QVERIFY(parseValueView(QString(), a, v, err));
    QCOMPARE(v, ValueView::Bit);
}

void tst_DeviceAddress::value_view_rejects_int16_on_m()
{
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("M1"), a);
    ValueView v;
    QString err;
    QVERIFY(!parseValueView(QStringLiteral("int16"), a, v, err));
}

void tst_DeviceAddress::toString_roundtrip()
{
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("d2024.03"), a));
    QCOMPARE(a.toString(), QStringLiteral("D2024.3"));
    QVERIFY(DeviceAddress::parse(QStringLiteral("M01500"), a));
    QCOMPARE(a.toString(), QStringLiteral("M1500"));
}
