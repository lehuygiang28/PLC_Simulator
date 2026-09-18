#ifndef TST_WATCHLIST_H
#define TST_WATCHLIST_H

#include <QObject>

class tst_WatchList : public QObject
{
    Q_OBJECT
private slots:
    void parse_mixed_ranges_and_singles();
    void parse_allows_repeated_prefix_on_range_end();
    void parse_skips_empty_tokens();
    void parse_rejects_cross_device_range();
    void parse_rejects_d_bit_range();
    void parse_rejects_inverted_range();
    void parse_rejects_too_many_items();
    void parse_rejects_empty_expression();
};

#endif
