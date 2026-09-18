#ifndef CONTROL_LOGRINGBUFFER_H
#define CONTROL_LOGRINGBUFFER_H

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QString>
#include <atomic>

struct LogEntry {
    qint64 id = 0;
    QString category;
    QDateTime timestamp;
    QJsonObject data;
};

class LogRingBuffer {
public:
    static constexpr int kDefaultCapacity = 500;

    explicit LogRingBuffer(int capacity = kDefaultCapacity);

    qint64 append(const QString& category, const QJsonObject& data);
    QList<LogEntry> since(qint64 afterId, int limit) const;
    qint64 latestId() const;

private:
    int m_capacity;
    mutable QMutex m_mutex;
    QList<LogEntry> m_entries;
    std::atomic<qint64> m_nextId{1};
};

#endif // CONTROL_LOGRINGBUFFER_H
