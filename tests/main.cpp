#include "tst_DeviceAddress.h"
#include "tst_WatchList.h"

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
    return status;
}
