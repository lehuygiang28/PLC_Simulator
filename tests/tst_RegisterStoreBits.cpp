#include "tst_RegisterStoreBits.h"
#include "Core/RegisterStore.h"
#include "Core/DeviceAddress.h"

#include <QtTest>

void tst_RegisterStoreBits::m_bit_roundtrip()
{
    RegisterStore store;
    DeviceAddress a;
    QVERIFY(DeviceAddress::parse(QStringLiteral("M1500"), a));
    QVERIFY(!store.GetBit(a));
    QVERIFY(store.SetBit(a, true));
    QVERIFY(store.GetBit(a));
    QCOMPARE(store.GetInt16(1500), int16_t(0));
}

void tst_RegisterStoreBits::d_bit_shares_d_word()
{
    RegisterStore store;
    DeviceAddress b;
    QVERIFY(DeviceAddress::parse(QStringLiteral("D10.3"), b));
    QVERIFY(store.SetBit(b, true));
    QCOMPARE(store.GetInt16(10), int16_t(1 << 3));
    store.SetInt16(10, 0);
    QVERIFY(!store.GetBit(b));
}

void tst_RegisterStoreBits::bulk_m_bits_and_m_words()
{
    RegisterStore store;
    std::vector<uint8_t> on = {1, 0, 1, 1};
    QVERIFY(store.setBits(DeviceKind::M, 32, on));
    const auto read = store.bits(DeviceKind::M, 32, 4);
    QCOMPARE(read, on);
    const auto words = store.words(DeviceKind::M, 32, 1);
    QCOMPARE(static_cast<int>(words.size()), 1);
    QCOMPARE(words[0], int16_t(0b1101));
}

void tst_RegisterStoreBits::m_word_requires_16_aligned_start()
{
    RegisterStore store;
    QVERIFY(store.words(DeviceKind::M, 1, 1).empty());
    QVERIFY(!store.setWords(DeviceKind::M, 1, {1}));
}

void tst_RegisterStoreBits::d_bit_bulk_spans_words()
{
    RegisterStore store;
    store.SetInt16(0, 0x0001);
    store.SetInt16(1, 0x0001);
    const auto bits = store.bits(DeviceKind::D, 0, 17);
    QCOMPARE(static_cast<int>(bits.size()), 17);
    QCOMPARE(bits[0], uint8_t(1));
    QCOMPARE(bits[16], uint8_t(1));
}

void tst_RegisterStoreBits::resetAll_clears_m_bits()
{
    RegisterStore store;
    DeviceAddress a;
    DeviceAddress::parse(QStringLiteral("M0"), a);
    store.SetBit(a, true);
    store.resetAll(7);
    QVERIFY(!store.GetBit(a));
    QCOMPARE(store.GetInt16(0), int16_t(7));
}
