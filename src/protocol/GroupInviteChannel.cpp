#include "GroupInviteChannel.h"
#include "Channel_p.h"
#include <QDebug>

using namespace Protocol;

GroupInviteChannel::GroupInviteChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.invite"), direction, connection)
    , m_inviteStatus(Incomplete)
{

}

bool GroupInviteChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    return true;
}

bool GroupInviteChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    return true;
}

void GroupInviteChannel::receivePacket(const QByteArray &packet)
{
    Data::GroupInvite::Packet message;
    if (!message.ParseFromArray(packet.constData(), packet.size()))
    {
        closeChannel();
        return;
    }
    qDebug() << "GroupInviteChannel::receivePacket got a packet";
}

bool GroupInviteChannel::sendInvite(QString text, QDateTime time)
{
    if (direction() != Outbound) {
        BUG() << "Group invite channels are unidirectional and this is not an outbound channel";
        return false;
    }
    QScopedPointer<Data::GroupInvite::Invite> message(new Data::GroupInvite::Invite);
    if (text.isEmpty()) {
        BUG() << "Invite message is empty, and it should've been discarded";
        return false;
    } else if (text.size() > MessageMaxCharacters) {
        BUG() << "Invite message is too long (" << text.size() << "chars), and it shoudl've been limited already. Truncated.";
        text.truncate(MessageMaxCharacters);
    }
    // Also converts to UTF-8
    message->set_message_text(text.toStdString());
    if (!time.isNull())
        message->set_timestamp(time.toMSecsSinceEpoch());
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite(message.take());
    if (!Channel::sendMessage(packet))
        return false;
    m_inviteStatus = Sent;
    return true;
}

void GroupInviteChannel::handleInvite(const Data::GroupInvite::Invite &invite)
{
    QScopedPointer<Data::GroupInvite::InviteResponse> response(new Data::GroupInvite::InviteResponse);
    QString text = QString::fromStdString(invite.message_text());
    qDebug() << "GroupInviteChannel::handleInvite got " << text;
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        response->set_accepted(false);
    } else if (text.isEmpty()) {
        qWarning() << "Rejected empty group invite message";
        response->set_accepted(false);
    } else if (text.size() > MessageMaxCharacters) {
        qWarning() << "Rejected oversize group invite message of " << text.size() << "chars";
        response->set_accepted(false);
    } else {
        QDateTime time = QDateTime::fromMSecsSinceEpoch(invite.timestamp());
        response->set_accepted(true);
    }
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite_response(response.take());
    Channel::sendMessage(packet);
}

void GroupInviteChannel::handleInviteResponse(const Data::GroupInvite::InviteResponse &response)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound response on an oubout group invite channel";
        closeChannel();
        return;
    }
    if (m_inviteStatus != Sent) {
        qWarning() << "I don't know why I received a group invite response";
        closeChannel();
        return;
    }
    if (response.accepted()) {
        m_inviteStatus = Accepted;
        qDebug() << "GroupInviteChannel response was good";
    }
    else {
        m_inviteStatus = Rejected;
        qDebug() << "GroupInviteChannel response was bad";
    }
    emit responseReceived(m_inviteStatus);
    closeChannel();

}
