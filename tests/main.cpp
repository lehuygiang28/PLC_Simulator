#include "tst_DeviceAddress.h"

#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;
    {
        tst_DeviceAddress tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
