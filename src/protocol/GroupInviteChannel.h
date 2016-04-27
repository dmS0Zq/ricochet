#ifndef PROTOCOL_GROUPINVITECHANNEL_H
#define PROTOCOL_GROUPINVITECHANNEL_H

#include "Channel.h"
#include "GroupInviteChannel.pb.h"
#include "core/Group.h"
#include "core/GroupsManager.h"
#include <QDateTime>
#include <QSet>

namespace Protocol
{
class GroupInviteChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupInviteChannel)
public:
    static const int MessageMaxCharacters = 2000;
    explicit GroupInviteChannel(Direction direction, Connection *connection);
    bool sendInvite(Data::GroupInvite::Invite invite);
    bool sendInviteResponse(Data::GroupInvite::InviteResponse response);
    bool sendIntroductionAccepted(Data::GroupInvite::IntroductionAccepted accepted);
signals:
    void introductionAcceptedReceived(const Data::GroupInvite::IntroductionAccepted &accepted);
    void inviteResponseReceived(const Data::GroupInvite::InviteResponse &response);
    void inviteReceived(const Data::GroupInvite::Invite &invite);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    void handleInvite(const Data::GroupInvite::Invite &invite);
    void handleInviteResponse(const Data::GroupInvite::InviteResponse &response);
    void handleIntroductionAccepted(const Data::GroupInvite::IntroductionAccepted &accepted);
};
}
#endif
