#ifndef TST_REGISTERTABLEMODEL_H
#define TST_REGISTERTABLEMODEL_H

#include <QObject>

class tst_RegisterTableModel : public QObject
{
    Q_OBJECT
private slots:
    void rows_follow_watch_items();
    void edit_m_bit();
    void edit_d_word_int16();
};

#endif
