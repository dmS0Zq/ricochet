#include "GroupMessageHistory.h"

#include <QDateTime>
#include <QDebug>

using Protocol::Data::GroupChat::GroupMessage;

GroupMessageHistory::GroupMessageHistory()
    : m_epoch(QByteArray(32, 0x0))
{
}

void GroupMessageHistory::insert(GroupMessage message)
{
    auto hash = hashMessage(message);
    m_history.insert(hash, message);
    m_insertOrder.insert(hash, m_history.size()-1);
    qDebug() << "epoch:" << m_epoch.toHex();
    qDebug() << m_history.size() << combine(m_history.size()).toHex();
    qDebug() << m_history.size()-1 << combine(m_history.size()-1).toHex();
}

bool GroupMessageHistory::contains(const GroupMessage &message)
{
    return m_history.keys().contains(hashMessage(message));
}

void GroupMessageHistory::reset()
{
    m_epoch = combine(m_history.size());
    m_history.clear();
    m_insertOrder.clear();
}

QByteArray GroupMessageHistory::hashMessage(const GroupMessage &message)
{
    int64_t timestamp = message.timestamp();
    QByteArray data; // author + timestamp + message_text
    data += QByteArray::fromStdString(message.author());
    data += QByteArray::fromRawData((char*)&timestamp, sizeof(int64_t));
    data += QByteArray::fromStdString(message.message_text());
    QByteArray hash = QCryptographicHash::hash(data, HashMethod);
    return hash;
}

QByteArray GroupMessageHistory::combine(int number)
{
    if (number > m_history.size()) {
        qWarning() << "Cannot combine" << number << "messages because there's only" << m_history.size() << ". Returning empty QByteArray";
        return QByteArray();
    }
    QByteArray final = QByteArray();
    foreach (auto message, m_history) {
        if (m_insertOrder[hashMessage(message)] >= number) continue;
        auto data = message.SerializeAsString();
        final += QByteArray::fromRawData(data.data(), data.size());
        final = QCryptographicHash::hash(final, GroupMessageHistory::HashMethod);
    }
    return final;
}
