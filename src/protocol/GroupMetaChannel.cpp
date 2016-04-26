#include "GroupMetaChannel.h"
#include "Channel_p.h"

#include <QDebug>

using namespace Protocol;

GroupMetaChannel::GroupMetaChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.meta"), direction, connection)
{

}

bool GroupMetaChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    return true;
}

bool GroupMetaChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
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
    Data::GroupMeta::Introduction *final = new Data::GroupMeta::Introduction();
    final->set_author(introduction.author());
    final->set_message_id(introduction.message_id());
    final->set_message_text(introduction.message_text());
    final->set_signature(introduction.signature());
    final->set_timestamp(introduction.timestamp());
    Data::GroupMeta::Packet packet;
    packet.set_allocated_introduction(final);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

bool GroupMetaChannel::sendIntroductionResponse(Data::GroupMeta::IntroductionResponse introductionResponse)
{
    Data::GroupMeta::IntroductionResponse *final = new Data::GroupMeta::IntroductionResponse();
    Data::GroupMeta::Packet packet;
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

void GroupMetaChannel::handleIntroduction(const Data::GroupMeta::Introduction &introduction)
{
    emit introductionReceived(introduction);
}

void GroupMetaChannel::handleIntroductionResponse(const Data::GroupMeta::IntroductionResponse &introductionResponse)
{
    emit introductionResponseReceived(introductionResponse);
}
