#ifndef GROUPMETACHANNEL_H
#define GROUPMETACHANNEL_H

#include "Channel.h"
#include "protocol/GroupMetaChannel.pb.h"

namespace Protocol {

class GroupMetaChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupMetaChannel)
public:
    explicit GroupMetaChannel(Direction direction, Connection *connection);
    bool sendIntroduction(Data::GroupMeta::Introduction introduction);
    bool sendIntroductionResponse(Data::GroupMeta::IntroductionResponse introductionResponse);
signals:
    void introductionReceived(Data::GroupMeta::Introduction introduction);
    void introductionResponseReceived(Data::GroupMeta::IntroductionResponse introductionResponse);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    void handleIntroduction(const Data::GroupMeta::Introduction &introduction);
    void handleIntroductionResponse(const Data::GroupMeta::IntroductionResponse &introductionResponse);
};

}
#endif // GROUPMETACHANNEL_H
