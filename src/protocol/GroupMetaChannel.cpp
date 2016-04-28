#include "GroupMetaChannel.h"
#include "Channel_p.h"
#include "core/GroupsManager.h"

#include <QDebug>

using namespace Protocol;

GroupMetaChannel::GroupMetaChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.meta"), direction, connection)
{

}

bool GroupMetaChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    Q_UNUSED(request)
    Q_UNUSED(result)
    return true;
}

bool GroupMetaChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    Q_UNUSED(request)
    return true;
}

void GroupMetaChannel::receivePacket(const QByteArray &packet)
{
    Data::GroupMeta::Packet p;
    if (!p.ParseFromArray(packet.constData(), packet.size()))
    {
        closeChannel();
        return;
    }
    if (p.has_introduction())
        handleIntroduction(p.introduction());
    else if (p.has_introduction_response())
        handleIntroductionResponse(p.introduction_response());
    else {
        qWarning() << "Unrecognized message on" << type();
        closeChannel();
    }
}

bool GroupMetaChannel::sendIntroduction(Data::GroupMeta::Introduction introduction)
{
    Data::GroupInvite::InviteResponse *inviteResponse = new Data::GroupInvite::InviteResponse();
    inviteResponse->set_signature(introduction.invite_response().signature());
    inviteResponse->set_timestamp(introduction.invite_response().timestamp());
    inviteResponse->set_accepted(introduction.invite_response().accepted());
    inviteResponse->set_author(introduction.invite_response().author());
    inviteResponse->set_public_key(introduction.invite_response().public_key());
    inviteResponse->set_message_text(introduction.invite_response().message_text());
    Data::GroupMeta::Introduction *final = new Data::GroupMeta::Introduction();
    final->set_allocated_invite_response(inviteResponse);
    final->set_signature(introduction.signature());
    final->set_timestamp(introduction.timestamp());
    final->set_author(introduction.author());
    final->set_message_text(introduction.message_text());
    Data::GroupMeta::Packet packet;
    packet.set_allocated_introduction(final);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

bool GroupMetaChannel::sendIntroductionResponse(Data::GroupMeta::IntroductionResponse introductionResponse)
{
    Data::GroupMeta::IntroductionResponse *final = new Data::GroupMeta::IntroductionResponse();
    final->set_signature(introductionResponse.signature());
    final->set_timestamp(introductionResponse.timestamp());
    final->set_accepted(introductionResponse.accepted());
    final->set_author(introductionResponse.author());
    Data::GroupMeta::Packet packet;
    packet.set_allocated_introduction_response(final);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

void GroupMetaChannel::handleIntroduction(const Data::GroupMeta::Introduction &introduction)
{
    Group *group = groupsManager->groupFromChannel(this);
    if (!group) {
        BUG() << "Unknown group for meta channel"<<identifier()<<". Where did this packet come from?";
        return;
    }
    if (!group->verifyPacket(introduction)) {
        qWarning() << "GroupMetaChannel::handleIntroduction ignoring intro that didn't verify";
        return;
    }
    emit introductionReceived(introduction);
}

void GroupMetaChannel::handleIntroductionResponse(const Data::GroupMeta::IntroductionResponse &introductionResponse)
{
    emit introductionResponseReceived(introductionResponse);
}
