#ifndef GROUPMESSAGEHISTORY_H
#define GROUPMESSAGEHISTORY_H

#include "protocol/GroupChatChannel.pb.h"

#include <QMap>

class GroupMessageHistory
{
public:
    void insert(Protocol::Data::GroupChat::GroupMessage message);
private:
    QMap<QByteArray, Protocol::Data::GroupChat::GroupMessage> m_history;
    void removeOldest();
    QByteArray keyOfOldest();
    void prune();
    static const int MinimumMessageCount = 5;
};

#endif // GROUPMESSAGEHISTORY_H
