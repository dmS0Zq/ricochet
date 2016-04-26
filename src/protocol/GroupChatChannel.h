#ifndef PROTOCOL_GROUPCHATCHANNEL_H
#define PROTOCOL_GROUPCHATCHANNEL_H

#include "Channel.h"
#include "GroupChatChannel.pb.h"
#include "core/Group.h"
#include <QDateTime>
#include <QHash>

namespace Protocol
{
class GroupChatChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupChatChannel)
public:
    static const int MessageMaxCharacters = 2000;
    explicit GroupChatChannel(Direction direction, Connection *connection);
    bool sendGroupMessage(Protocol::Data::GroupChat::GroupMessage message);
signals:
    void messageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, bool accepted);
    void groupMessageReceived(const Protocol::Data::GroupChat::GroupMessage &message);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    QHash<unsigned int, Protocol::Data::GroupChat::GroupMessage> pendingMessages;
    void handleGroupMessage(const Data::GroupChat::GroupMessage &message);
    void handleGroupAcknowledge(const Data::GroupChat::GroupMessageAcknowledge &message);
};
}
#endif
