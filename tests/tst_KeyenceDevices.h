#ifndef TST_KEYENCEDEVICES_H
#define TST_KEYENCEDEVICES_H

#include <QObject>

class tst_KeyenceDevices : public QObject
{
    Q_OBJECT
private slots:
    void read_dm_word_unchanged();
    void read_mr_bits();
    void write_mr_bits();
    void pack_mr_read();
};

#endif
