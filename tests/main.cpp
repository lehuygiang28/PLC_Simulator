#include "tst_DeviceAddress.h"
#include "tst_WatchList.h"
#include "tst_RegisterStoreBits.h"
#include "tst_MitsubishiDevices.h"
#include "tst_KeyenceDevices.h"

#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;
    {
        tst_DeviceAddress tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        tst_WatchList tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        tst_RegisterStoreBits tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        tst_MitsubishiDevices tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        tst_KeyenceDevices tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
