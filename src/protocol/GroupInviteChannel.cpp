#include "GroupInviteChannel.h"
#include "Channel_p.h"
#include "core/GroupMember.h"
#include "core/UserIdentity.h"
#include "core/IdentityManager.h"
#include "tor/HiddenService.h"
#include <QDebug>

using namespace Protocol;

GroupInviteChannel::GroupInviteChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.invite"), direction, connection)
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
    if (message.has_invite())
        handleInvite(message.invite());
    else if (message.has_invite_response())
        handleInviteResponse(message.invite_response());
}

bool GroupInviteChannel::sendInvite(Data::GroupInvite::Invite invite)
{
    if (direction() != Outbound) {
        BUG() << "Group invite channels are unidirectional and this is not an outbound channel";
        return false;
    }
    Data::GroupInvite::Invite *final = new Data::GroupInvite::Invite();
    final->set_signature(invite.signature());
    final->set_timestamp(invite.timestamp());
    final->set_message_text(invite.message_text());
    final->set_author(invite.author());
    final->set_public_key(invite.public_key());
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite(final);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

bool GroupInviteChannel::sendInviteResponse(Data::GroupInvite::InviteResponse response)
{
    if (direction() != Inbound) {
        BUG() << "Cannot send invite response on a channel which is not inbound";
        return false;
    }
    Data::GroupInvite::InviteResponse *final = new Data::GroupInvite::InviteResponse();
    final->set_signature(response.signature());
    final->set_timestamp(response.timestamp());
    final->set_accepted(response.accepted());
    final->set_author(response.author());
    final->set_public_key(response.public_key());
    final->set_message_text(response.message_text());
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite_response(final);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

void GroupInviteChannel::handleInvite(const Data::GroupInvite::Invite &invite)
{
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        return;
    }
    Group *group = groupsManager->test_getTestingGroup();
    //Group *group = groupsManager->groupFromChannel(this);
    if (!group) {
        BUG() << "Unknown group for invite channel" << identifier() << ". Where did this packet come from?";
        return;
    }
    ContactUser *contact = group->selfIdentity()->getContacts()->contactUserFromChannel(this);
    GroupMember *member = new GroupMember(contact);
    member->connectIncomingSignals();
    group->addGroupMember(member);
    if (group->verifyPacket(invite)) {
        emit inviteReceived(invite);
    }
    else {
        qWarning() << "GroupInviteChannel::handleInvite ignoring invite that didn't verify";
    }
}

void GroupInviteChannel::handleInviteResponse(const Data::GroupInvite::InviteResponse &response)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound response on an oubout group invite channel";
        closeChannel();
        return;
    }
    Group *group = groupsManager->test_getTestingGroup();
    if (!group) {
        BUG() << "Unknown group for invite channel" << identifier() << ". Where did this packet come from?";
        return;
    }
    if (group->verifyPacket(response)) {
        emit inviteAcknowleged(response);
    }
    closeChannel();
}
