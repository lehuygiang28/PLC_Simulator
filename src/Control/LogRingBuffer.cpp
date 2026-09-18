#include "LogRingBuffer.h"

LogRingBuffer::LogRingBuffer(int capacity)
    : m_capacity(capacity > 0 ? capacity : kDefaultCapacity)
{
}

qint64 LogRingBuffer::append(const QString& category, const QJsonObject& data)
{
    LogEntry entry;
    entry.id = m_nextId.fetch_add(1);
    entry.category = category;
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.data = data;

    QMutexLocker locker(&m_mutex);
    m_entries.append(entry);
    while (m_entries.size() > m_capacity)
        m_entries.removeFirst();
    return entry.id;
}

QList<LogEntry> LogRingBuffer::since(qint64 afterId, int limit) const
{
    if (limit <= 0)
        limit = m_capacity;

    QMutexLocker locker(&m_mutex);
    QList<LogEntry> out;
    for (const LogEntry& entry : m_entries) {
        if (entry.id <= afterId)
            continue;
        out.append(entry);
        if (out.size() >= limit)
            break;
    }
    return out;
}

qint64 LogRingBuffer::latestId() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries.isEmpty() ? 0 : m_entries.last().id;
}
