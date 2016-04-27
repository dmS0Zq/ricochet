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
    connect(this, &GroupInviteChannel::inviteReceived, groupsManager, &GroupsManager::onInviteReceived);
    connect(this, &GroupInviteChannel::introductionAcceptedReceived, groupsManager, &GroupsManager::onIntroductionAcceptedReceived);
}

bool GroupInviteChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    Q_UNUSED(request)
    Q_UNUSED(result)
    return true;
}

bool GroupInviteChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    Q_UNUSED(request)
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
    else if (message.has_introduction_accepted())
        handleIntroductionAccepted(message.introduction_accepted());
}

bool GroupInviteChannel::sendInvite(Data::GroupInvite::Invite invite)
{
    if (direction() != Outbound) {
        BUG() << "Group invite channels are unidirectional and this is not an outbound channel";
        return false;
    }
    if (!Group::verifyPacket(invite)) {
        qWarning() << "GroupInviteChannel::sendInvite refusing to send invite that didn't verify";
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
    if (!Group::verifyPacket(response)) {
        qWarning() << "GroupInviteChannel::sendInviteResponse refusing to send packet that didn't verify";
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

bool GroupInviteChannel::sendIntroductionAccepted(Data::GroupInvite::IntroductionAccepted accepted)
{
    if (direction() != Outbound) {
        BUG() << "Cannot send introduction accepted on a channel which is not outbound";
        return false;
    }
    if (!Group::verifyPacket(accepted)) {
        qWarning() << "GroupInviteChannel::sendIntroductionAccepted refusing to send packet that did not verify";
        return false;
    }
    Data::GroupInvite::InviteResponse *finalResponse = new Data::GroupInvite::InviteResponse();
    finalResponse->set_signature(accepted.invite_response().signature());
    finalResponse->set_timestamp(accepted.invite_response().timestamp());
    finalResponse->set_accepted(accepted.invite_response().accepted());
    finalResponse->set_author(accepted.invite_response().author());
    finalResponse->set_public_key(accepted.invite_response().public_key());
    finalResponse->set_message_text(accepted.invite_response().message_text());
    Data::GroupInvite::IntroductionAccepted *final = new Data::GroupInvite::IntroductionAccepted();
    final->set_allocated_invite_response(finalResponse);
    final->set_signature(accepted.signature());
    final->set_timestamp(accepted.timestamp());
    final->set_author(accepted.author());
    for (int i = 0; i < accepted.member_size(); i++) {
        final->add_member(accepted.member(i));
        final->add_member_public_key(accepted.member_public_key(i));
        final->add_member_signature(accepted.member_signature(i));
    }
    Data::GroupInvite::Packet packet;
    packet.set_allocated_introduction_accepted(final);
    if (!Channel::sendMessage(packet)) {
        return false;
    }
    return true;
}

void GroupInviteChannel::handleInvite(const Data::GroupInvite::Invite &invite)
{
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        return;
    }
    if (!Group::verifyPacket(invite)) {
        qWarning() << "Rejected invite which did not verify";
        return;
    }
    emit inviteReceived(invite);
}

void GroupInviteChannel::handleInviteResponse(const Data::GroupInvite::InviteResponse &response)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound response on an oubout group invite channel";
        closeChannel();
        return;
    }
    if (!Group::verifyPacket(response)) {
        qWarning() << "Rejected invite response which did not verify";
    }
    emit inviteResponseReceived(response);
}

void GroupInviteChannel::handleIntroductionAccepted(const Data::GroupInvite::IntroductionAccepted &accepted)
{
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        closeChannel();
        return;
    }
    if (!Group::verifyPacket(accepted)) {
        qWarning() << "Rejected introduction accepted which did not verify";
        closeChannel();
        return;
    }
    emit introductionAcceptedReceived(accepted);
    closeChannel();
}
