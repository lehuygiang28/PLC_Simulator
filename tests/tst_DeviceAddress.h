#ifndef TST_DEVICEADDRESS_H
#define TST_DEVICEADDRESS_H

#include <QObject>

class tst_DeviceAddress : public QObject
{
    Q_OBJECT
private slots:
    void parse_accepts_d_word();
    void parse_accepts_m_bit();
    void parse_accepts_d_bit();
    void parse_is_case_insensitive();
    void parse_rejects_unknown_device();
    void parse_rejects_out_of_range();
    void parse_rejects_bad_d_bit();
    void toString_roundtrip();
    void value_view_defaults_bit_for_m();
    void value_view_rejects_int16_on_m();
};

#endif
