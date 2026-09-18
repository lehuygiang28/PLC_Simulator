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
    void segmented_layout_starts_each_comma_token_in_new_column_pair();
    void m_and_d_use_distinct_watch_colors();
};

#endif
