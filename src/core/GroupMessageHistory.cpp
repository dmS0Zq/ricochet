#include "GroupMessageHistory.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

void GroupMessageHistory::insert(Protocol::Data::GroupChat::GroupMessage message)
{
    QByteArray key = QCryptographicHash::hash(QString::fromStdString(message.message_text()).toUtf8() + QDateTime::fromMSecsSinceEpoch(message.timestamp()).toUTC().toString().toUtf8(), QCryptographicHash::Sha256);
    qDebug() << key.size();
    m_history.insert(key, message);
    if (m_history.size() > GroupMessageHistory::MaxMessageCount) {
        removeOldest();
    }
    //foreach (auto m, m_history)
    //    qDebug() << QDateTime::fromMSecsSinceEpoch(m.timestamp()).toLocalTime().toString();
    //qDebug() << "\n";
}

void GroupMessageHistory::removeOldest()
{
    if (m_history.size() < 1)
        return;
    QByteArray old;
    for (auto it = m_history.begin(); it != m_history.end(); it++) {
        if (old == QByteArray())
            old = it.key();
        else if ((*it).timestamp() < m_history[old].timestamp()) {
            old = it.key();
        }
    }
    m_history.remove(old);
}
