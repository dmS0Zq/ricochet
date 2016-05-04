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
    bool sendLeave(Data::GroupMeta::Leave &leave);
signals:
    void introductionReceived(Data::GroupMeta::Introduction introduction);
    void introductionResponseReceived(Data::GroupMeta::IntroductionResponse introductionResponse);
    void leaveReceived(Data::GroupMeta::Leave leave);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    void handleIntroduction(const Data::GroupMeta::Introduction &introduction);
    void handleIntroductionResponse(const Data::GroupMeta::IntroductionResponse &introductionResponse);
    void handleLeave(const Data::GroupMeta::Leave &leave);
};

}
#endif // GROUPMETACHANNEL_H
