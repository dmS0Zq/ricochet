#include "GroupMessageHistory.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

void GroupMessageHistory::insert(Protocol::Data::GroupChat::GroupMessage message)
{
    QByteArray key = QCryptographicHash::hash(QString::fromStdString(message.message_text()).toUtf8() + QDateTime::fromMSecsSinceEpoch(message.timestamp()).toUTC().toString().toUtf8(), QCryptographicHash::Sha256);
    m_history.insert(key, message);
    //qDebug() << m_history.size() << "messages" << "and oldest from" << m_history[keyOfOldest()].timestamp();
    prune();
    //foreach (auto m, m_history)
    //    qDebug() << QDateTime::fromMSecsSinceEpoch(m.timestamp()).toLocalTime().toString();
    //qDebug() << "\n";
}

QByteArray GroupMessageHistory::keyOfOldest()
{
    QByteArray old;
    for (auto it = m_history.begin(); it != m_history.end(); it++) {
        if (old == QByteArray())
            old = it.key();
        else if ((*it).timestamp() < m_history[old].timestamp()) {
            old = it.key();
        }
    }
    return old;
}

void GroupMessageHistory::prune()
{
    if (m_history.size() <= GroupMessageHistory::MinimumMessageCount)
        return;
    auto now = QDateTime::currentDateTimeUtc();
    auto oldest = keyOfOldest();
    auto oldMessage = m_history[oldest];
    auto oldMessageTimestamp = QDateTime::fromMSecsSinceEpoch(oldMessage.timestamp()).toUTC();
    while (oldMessageTimestamp.addSecs(15) < now
        && m_history.size() > GroupMessageHistory::MinimumMessageCount)
    {
        m_history.remove(oldest);
        oldest = keyOfOldest();
        oldMessage = m_history[oldest];
        oldMessageTimestamp = QDateTime::fromMSecsSinceEpoch(oldMessage.timestamp()).toUTC();
    }
}
