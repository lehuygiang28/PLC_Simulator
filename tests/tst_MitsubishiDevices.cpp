#include "tst_MitsubishiDevices.h"
#include "Comm/Protocol/CommProtocolMitsubishiQBinary.h"
#include "Comm/Protocol/PlcAccess.h"
#include "Core/DeviceAddress.h"

#include <QtTest>

static QByteArray mcRequest(uint16_t cmd, uint16_t sub, uint32_t devNo, uint8_t devCode, uint16_t points,
                            const QByteArray& payload = QByteArray())
{
    QByteArray body;
    auto appendLe16 = [&](uint16_t v) {
        body.append(static_cast<char>(v & 0xFF));
        body.append(static_cast<char>((v >> 8) & 0xFF));
    };
    appendLe16(0x0010);
    appendLe16(cmd);
    appendLe16(sub);
    body.append(static_cast<char>(devNo & 0xFF));
    body.append(static_cast<char>((devNo >> 8) & 0xFF));
    body.append(static_cast<char>((devNo >> 16) & 0xFF));
    body.append(static_cast<char>(devCode));
    appendLe16(points);
    body.append(payload);

    QByteArray pkt = QByteArray::fromHex("500000FFFF0300");
    const uint16_t len = static_cast<uint16_t>(body.size());
    pkt.append(static_cast<char>(len & 0xFF));
    pkt.append(static_cast<char>((len >> 8) & 0xFF));
    pkt.append(body);
    return pkt;
}

void tst_MitsubishiDevices::read_d_word_still_works()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    const QByteArray req = mcRequest(0x0401, 0x0000, 100, 0xA8, 1);
    CmdType t = CmdType::eCmdUnkown;
    QVERIFY(pro.AnalyzeCmdInfo(req, t));
    QCOMPARE(t, CmdType::eCmdReadReg);
    QVERIFY(pro.AnalyzeReadReg(req, a));
    QCOMPARE(a.device, DeviceKind::D);
    QCOMPARE(a.unit, PlcUnit::Word);
    QCOMPARE(a.start, 100);
    QCOMPARE(a.count, 1);
}

void tst_MitsubishiDevices::read_m_bit()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    const QByteArray req = mcRequest(0x0401, 0x0001, 1500, 0x90, 2);
    QVERIFY(pro.AnalyzeReadReg(req, a));
    QCOMPARE(a.device, DeviceKind::M);
    QCOMPARE(a.unit, PlcUnit::Bit);
    QCOMPARE(a.start, 1500);
    QCOMPARE(a.count, 2);
}

void tst_MitsubishiDevices::write_m_bit_payload()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    QByteArray payload;
    payload.append(char(0x01));
    payload.append(char(0x00));
    const QByteArray req = mcRequest(0x1401, 0x0001, 1564, 0x90, 2, payload);
    CmdType t = CmdType::eCmdUnkown;
    QVERIFY(pro.AnalyzeCmdInfo(req, t));
    QCOMPARE(t, CmdType::eCmdWriteReg);
    QVERIFY(pro.AnalyzeWriteReg(req, a));
    QCOMPARE(a.device, DeviceKind::M);
    QCOMPARE(a.start, 1564);
    QCOMPARE(a.count, 2);
    QCOMPARE(static_cast<int>(a.bitData.size()), 2);
    QCOMPARE(a.bitData[0], uint8_t(1));
    QCOMPARE(a.bitData[1], uint8_t(0));
}

void tst_MitsubishiDevices::pack_bit_read_pads_odd_count()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    a.device = DeviceKind::M;
    a.unit = PlcUnit::Bit;
    a.count = 1;
    a.bitData = {1};
    QByteArray reply;
    QVERIFY(pro.PackReportReadRegInfo(reply, a));
    QVERIFY(!reply.isEmpty());
    const QByteArray hex = reply.toHex().toUpper();
    QVERIFY(hex.contains("0100"));
}

void tst_MitsubishiDevices::read_d_bit_unit()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    const QByteArray req = mcRequest(0x0401, 0x0001, 2024, 0xA8, 16);
    QVERIFY(pro.AnalyzeReadReg(req, a));
    QCOMPARE(a.device, DeviceKind::D);
    QCOMPARE(a.unit, PlcUnit::Bit);
    QCOMPARE(a.start, 2024);
    QCOMPARE(a.count, 16);
}

void tst_MitsubishiDevices::m_word_rejects_unaligned()
{
    CommProtocolMitsubishiQBinary pro(nullptr);
    PlcAccess a;
    const QByteArray req = mcRequest(0x0401, 0x0000, 1500, 0x90, 1);
    QVERIFY(!pro.AnalyzeReadReg(req, a));
}
