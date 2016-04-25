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
    const QByteArray oldest();
    void removeOldest();
    static const int MaxMessageCount = 5;
};

#endif // GROUPMESSAGEHISTORY_H
