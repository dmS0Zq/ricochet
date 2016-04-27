#ifndef GROUPINVITEMONITOR_H
#define GROUPINVITEMONITOR_H

#include "core/Group.h"
#include "core/GroupMember.h"
#include "protocol/GroupInviteChannel.pb.h"

#include <QObject>
#include <QTimer>
#include <QList>

class Group;

class GroupInviteMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupInviteMonitor)

public:
    GroupInviteMonitor(Protocol::Data::GroupInvite::Invite &invite, GroupMember *invitee, GroupMember *inviter, Group *group, QObject *parent = 0);

    const Protocol::Data::GroupInvite::Invite invite() { return m_invite; }
    const Protocol::Data::GroupInvite::InviteResponse response() { return m_response; }
    GroupMember *invitee() { return m_invitee; }
    bool go();
signals:
    void inviteMonitorDone(GroupInviteMonitor *monitor, bool responseReceived);
public slots:
    void onInviteResponseReceived(const Protocol::Data::GroupInvite::InviteResponse &response);
private slots:
    void onTimeout();
private:
    Protocol::Data::GroupInvite::Invite m_invite;
    Protocol::Data::GroupInvite::InviteResponse m_response;
    GroupMember *m_invitee;
    GroupMember *m_inviter;
    QTimer m_timer;
    Group *m_group;
};

#endif // GROUPINVITEMONITOR_H
