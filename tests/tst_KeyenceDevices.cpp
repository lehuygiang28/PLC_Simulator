#include "tst_KeyenceDevices.h"
#include "Comm/Protocol/CommProtocolKeyencePCLink.h"
#include "Comm/Protocol/PlcAccess.h"

#include <QtTest>

void tst_KeyenceDevices::read_dm_word_unchanged()
{
    CommProtocolKeyencePCLink pro;
    PlcAccess a;
    const QByteArray req = "RDS DM00100.H 0001";
    QVERIFY(pro.AnalyzeReadReg(req, a));
    QCOMPARE(a.device, DeviceKind::D);
    QCOMPARE(a.unit, PlcUnit::Word);
    QCOMPARE(a.start, 100);
    QCOMPARE(a.count, 1);
}

void tst_KeyenceDevices::read_mr_bits()
{
    CommProtocolKeyencePCLink pro;
    PlcAccess a;
    const QByteArray req = "RDS MR01500 0002";
    CmdType t = CmdType::eCmdUnkown;
    QVERIFY(pro.AnalyzeCmdInfo(req, t));
    QCOMPARE(t, CmdType::eCmdReadReg);
    QVERIFY(pro.AnalyzeReadReg(req, a));
    QCOMPARE(a.device, DeviceKind::M);
    QCOMPARE(a.unit, PlcUnit::Bit);
    QCOMPARE(a.start, 1500);
    QCOMPARE(a.count, 2);
}

void tst_KeyenceDevices::write_mr_bits()
{
    CommProtocolKeyencePCLink pro;
    PlcAccess a;
    const QByteArray req = "WRS MR01564 0002 1 0";
    QVERIFY(pro.AnalyzeWriteReg(req, a));
    QCOMPARE(a.device, DeviceKind::M);
    QCOMPARE(a.start, 1564);
    QCOMPARE(static_cast<int>(a.bitData.size()), 2);
    QCOMPARE(a.bitData[0], uint8_t(1));
    QCOMPARE(a.bitData[1], uint8_t(0));
}

void tst_KeyenceDevices::pack_mr_read()
{
    CommProtocolKeyencePCLink pro;
    PlcAccess a;
    a.device = DeviceKind::M;
    a.unit = PlcUnit::Bit;
    a.bitData = {1, 0, 1};
    a.count = 3;
    QByteArray reply;
    QVERIFY(pro.PackReportReadRegInfo(reply, a));
    QCOMPARE(reply, QByteArray("1 0 1"));
}
