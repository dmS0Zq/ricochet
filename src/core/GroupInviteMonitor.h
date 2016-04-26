#ifndef GROUPINVITEMONITOR_H
#define GROUPINVITEMONITOR_H

#include "core/GroupMember.h"
#include "protocol/GroupInviteChannel.pb.h"

#include <QObject>
#include <QTimer>
#include <QList>

class GroupInviteMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupInviteMonitor)

public:
    GroupInviteMonitor(Protocol::Data::GroupInvite::Invite invite, GroupMember* member, QObject *parent = 0);

    Protocol::Data::GroupInvite::Invite invite() const { return m_invite; }
    Protocol::Data::GroupInvite::InviteResponse inviteResponse() const { return m_inviteResponse; }
signals:
    void inviteMonitorDone(GroupInviteMonitor *monitor, GroupMember *member, bool responseReceived);
public slots:
    void onGroupInviteAcknowledged(Protocol::Data::GroupInvite::InviteResponse inviteResponse);
private slots:
    void onTimeout();
private:
    Protocol::Data::GroupInvite::Invite m_invite;
    Protocol::Data::GroupInvite::InviteResponse m_inviteResponse;
    GroupMember *m_member;
};

#endif // GROUPINVITEMONITOR_H
