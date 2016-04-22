#ifndef PROTOCOL_GROUPCHATCHANNEL_H
#define PROTOCOL_GROUPCHATCHANNEL_H

#include "Channel.h"
#include "GroupChatChannel.pb.h"
#include "core/Group.h"
#include <QDateTime>
#include <QSet>

namespace Protocol
{
class GroupChatChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupChatChannel)
public:
    typedef quint32 MessageId;
    static const int MessageMaxCharacters = 2000;
    explicit GroupChatChannel(Direction direction, Connection *connection);
    bool sendGroupMessage(Protocol::Data::GroupChat::GroupMessage *message);
    bool sendGroupMessageWithId(Protocol::Data::GroupChat::GroupMessage *message, MessageId id);
signals:
    void messageAcknowledged(MessageId id, bool accepted);
    void messageReceived(const QString &text, const QDateTime &time, MessageId id);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    QSet<MessageId> pendingMessages;
    MessageId lastMessageId;
    void handleGroupMessage(const Data::GroupChat::GroupMessage &message);
    void handleGroupAcknowledge(const Data::GroupChat::GroupMessageAcknowledge &message);
};
}
#endif
