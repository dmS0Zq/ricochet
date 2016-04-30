#ifndef GROUPMESSAGEHISTORY_H
#define GROUPMESSAGEHISTORY_H

#include "protocol/GroupChatChannel.pb.h"

#include <QCryptographicHash>
#include <QMap>
#include <QHash>

using Protocol::Data::GroupChat::GroupMessage;

class GroupMessageHistory
{
public:
    GroupMessageHistory();

    void insert(GroupMessage message);
    bool contains(const GroupMessage &message);
    void reset();
private:
    QByteArray m_epoch;
    // stores the messages by their hashes. Iterating over a
    // QMap always shorts by key
    QMap<QByteArray, GroupMessage> m_history;
    // stores the order of which messages were inserted into
    // the map so that all but the last X inserted can be
    // pulled out even though there QMap sort order is different
    QHash<QByteArray, int> m_insertOrder;

    QByteArray combine(int number);

    static QByteArray hashMessage(const GroupMessage &message);
    static const auto HashMethod = QCryptographicHash::Sha256;
};

#endif // GROUPMESSAGEHISTORY_H
