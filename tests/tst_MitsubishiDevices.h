#ifndef TST_MITSUBISHIDEVICES_H
#define TST_MITSUBISHIDEVICES_H

#include <QObject>

class tst_MitsubishiDevices : public QObject
{
    Q_OBJECT
private slots:
    void read_d_word_still_works();
    void read_m_bit();
    void write_m_bit_payload();
    void pack_bit_read_pads_odd_count();
    void pack_bit_read_even_count_nibble_packed();
    void read_d_bit_unit();
    void m_word_rejects_unaligned();
};

#endif
