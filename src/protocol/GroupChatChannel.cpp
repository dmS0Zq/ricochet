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
    // The peer might use recent message IDs between connections to handle
    // re-send. Start at a random ID to reduce chance of collisions, then increment
    lastMessageId = SecureRNG::randomInt(UINT32_MAX);
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

bool GroupChatChannel::sendGroupMessage(Protocol::Data::GroupChat::GroupMessage *message)
{
    MessageId id = ++lastMessageId;
    return sendGroupMessageWithId(message, id);
}

bool GroupChatChannel::sendGroupMessageWithId(Protocol::Data::GroupChat::GroupMessage *message, MessageId id)
{
    if (direction() != Outbound) {
        BUG() << "Group chat channels are unidirectional and this is not an outbound channel";
        return false;
    }
    message->set_message_id(id);
    QString text = QString::fromStdString(message->message_text());
    if (text.isEmpty()) {
        BUG() << "Chat message is empty, and it should've been discarded";
        return false;
    } else if (text.size() > MessageMaxCharacters) {
        BUG() << "Chat message is too long (" << text.size() << "chars), and it shoudl've been limited already. Truncated.";
        text.truncate(MessageMaxCharacters);
        message->set_message_text(text.toStdString());
    }
    // Also converts to UTF-8
    Data::GroupChat::Packet p;
    p.set_allocated_group_message(message);
    if (!Channel::sendMessage(p))
        return false;
    pendingMessages.insert(id);
    return true;

}

void GroupChatChannel::handleGroupMessage(const Data::GroupChat::GroupMessage &message)
{
    QScopedPointer<Data::GroupChat::GroupMessageAcknowledge> response(new Data::GroupChat::GroupMessageAcknowledge);
    QString text = QString::fromStdString(message.message_text());
    QString author = QString::fromStdString(message.author());
    QDateTime time = QDateTime::fromMSecsSinceEpoch(message.timestamp()).toUTC();
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group chat channel";
        response->set_accepted(false);
    } else if (text.isEmpty()) {
        qWarning() << "Rejected empty group chat message";
        response->set_accepted(false);
    } else if (text.size() > MessageMaxCharacters) {
        qWarning() << "Rejected oversize group chat message of " << text.size() << "chars";
        response->set_accepted(false);
    } else if (!message.has_signature()) {
        qWarning() << "Rejected group chat message without signature";
        response->set_accepted(false);
    } else if (!message.has_author()) {
        qWarning() << "Rejected group chat message without author";
        response->set_accepted(false);
    } else {
        QDateTime time = QDateTime::fromMSecsSinceEpoch(message.timestamp());
        emit messageReceived(text, time, message.message_id());
        response->set_accepted(true);
    }
    if (message.has_message_id()) {
        response->set_message_id(message.message_id());
        Data::GroupChat::Packet packet;
        packet.set_allocated_group_message_acknowledge(response.take());
        Channel::sendMessage(packet);
    }
    Group *group = groupsManager->groupFromChannel(this);
    if (!group) {
        BUG() << "No group found with this channel ("<< identifier() <<"). Where did this packet come from?";
        return;
    }
    if (!group->verifyPacket(message)) {
        qDebug() << "☓" << time.toLocalTime().toString() <<  group->groupMembers()[author]->nickname() << text;
        //qDebug() << "GroupChatChannel::handleGroupMessage: packet did not verify";
    }
    else {
        qDebug() << "✓" << time.toLocalTime().toString() << group->groupMembers()[author]->nickname() << text;
        //qDebug() << "GroupChatChannel::handleGroupMessage: packet did verify";
    }
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
    MessageId id = message.message_id();
    if (pendingMessages.remove(id)) {
        emit messageAcknowledged(id, message.accepted());
    } else {
        qDebug() << "Received group chat ack for unknown message " << id;
    }
}
