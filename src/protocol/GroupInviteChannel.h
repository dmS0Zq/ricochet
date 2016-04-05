#ifndef PROTOCOL_GROUPINVITECHANNEL_H
#define PROTOCOL_GROUPINVITECHANNEL_H

#include "Channel.h"
#include "GroupInviteChannel.pb.h"
#include <QDateTime>
#include <QSet>

namespace Protocol
{
class GroupInviteChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupInviteChannel)
public:
    enum InviteStatus
    {
        Incomplete,
        Sent,
        Accepted,
        Rejected,
    };

    static const int MessageMaxCharacters = 2000;
    explicit GroupInviteChannel(Direction direction, Connection *connection);
    bool sendInvite(QString text, QDateTime time);
signals:
    void responseReceived(GroupInviteChannel::InviteStatus status);
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    void handleInvite(const Data::GroupInvite::Invite &invite);
    void handleInviteResponse(const Data::GroupInvite::InviteResponse &response);
    InviteStatus m_inviteStatus;
};
}
#endif
