#ifndef TST_REGISTERSTOREBITS_H
#define TST_REGISTERSTOREBITS_H

#include <QObject>

class tst_RegisterStoreBits : public QObject
{
    Q_OBJECT
private slots:
    void m_bit_roundtrip();
    void d_bit_shares_d_word();
    void bulk_m_bits_and_m_words();
    void m_word_requires_16_aligned_start();
    void d_bit_bulk_spans_words();
    void resetAll_clears_m_bits();
};

#endif
