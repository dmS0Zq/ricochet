#include "GroupChatChannel.h"
#include "Channel_p.h"
#include "core/Group.h"
#include "core/GroupsManager.h"
#include "core/UserIdentity.h"
#include "tor/HiddenService.h"
#include "utils/SecureRNG.h"
#include "utils/Useful.h"
#include "utils/CryptoKey.h"
#include <QDebug>

using namespace Protocol;

GroupChatChannel::GroupChatChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.chat"), direction, connection)
{
}

bool GroupChatChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    return true;
}

bool GroupChatChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    return true;
}

void GroupChatChannel::receivePacket(const QByteArray &packet)
{
    Data::GroupChat::Packet message;
    if (!message.ParseFromArray(packet.constData(), packet.size()))
    {
        closeChannel();
        return;
    }
    //qDebug() << "GroupChatChannel::receivePacket got a packet";
    if (message.has_group_message())
        handleGroupMessage(message.group_message());
    else if (message.has_group_message_acknowledge())
        handleGroupAcknowledge(message.group_message_acknowledge());
    else {
        qWarning() << "Unrecognized message on" << type();
        closeChannel();
    }
}

bool GroupChatChannel::sendGroupMessage(Protocol::Data::GroupChat::GroupMessage message)
{
    if (direction() != Outbound) {
        BUG() << "Group chat channels are unidirectional and this is not an outbound channel";
        return false;
    }
    Data::GroupChat::GroupMessage *final = new Protocol::Data::GroupChat::GroupMessage();
    final->set_message_id(message.message_id());
    final->set_signature(message.signature());
    final->set_timestamp(message.timestamp());
    final->set_author(message.author());
    final->set_message_text(message.message_text());
    Data::GroupChat::Packet packet;
    packet.set_allocated_group_message(final);
    if (!Channel::sendMessage(packet))
        return false;
    pendingMessages.insert(final->message_id(), *final);
    return true;
}

void GroupChatChannel::handleGroupMessage(const Data::GroupChat::GroupMessage &message)
{
    if (direction() != Inbound) {
        qWarning() << "GroupChatChannel::handleGroupMessage Rejected inbound message on an outbound group chat channel";
        return;
    }
    Group *group = groupsManager->groupFromChannel(this);
    if (!group) {
        BUG() << "Unknown group for chat channel"<<identifier()<<". Where did this packet come from?";
        return;
    }
    auto acknowledge = new Data::GroupChat::GroupMessageAcknowledge();
    if (group->verifyPacket(message)) {
        emit groupMessageReceived(message);
        acknowledge->set_accepted(true);
    }
    else {
        qWarning() << "GroupChatChannel::handleGroupMessage ignoring message that didn't verify";
        acknowledge->set_accepted(false);
    }
    acknowledge->set_message_id(message.message_id());
    Data::GroupChat::Packet packet;
    packet.set_allocated_group_message_acknowledge(acknowledge);
    Channel::sendMessage(packet);
}

void GroupChatChannel::handleGroupAcknowledge(const Data::GroupChat::GroupMessageAcknowledge &message)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound ack on an inbound group chat channel";
        closeChannel();
        return;
    }
    if (!message.has_message_id()) {
        qDebug() << "Group chat ack doesn't have a message ID we understand";
        closeChannel();
        return;
    }
    unsigned int id = message.message_id();
    if (pendingMessages.contains(id)) {
        auto m = pendingMessages[id];
        pendingMessages.remove(id);
        emit messageAcknowledged(m, message.accepted());
    }
    else {
        qDebug() << "Received group chat ack for unknown message " << id;
    }
}
