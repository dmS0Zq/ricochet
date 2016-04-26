#include "GroupInviteMonitor.h"

#include <QDebug>
#include <QTimer>

using Protocol::Data::GroupChat::GroupMessage;

GroupInviteMonitor::GroupInviteMonitor(Protocol::Data::GroupInvite::Invite invite, GroupMember* member, QObject *parent)
    : QObject(parent)
{
    m_invite = invite;
    m_member = member;
    QTimer::singleShot(10*1000, this, &GroupInviteMonitor::onTimeout);
}

void GroupInviteMonitor::onTimeout()
{
    emit inviteMonitorDone(this, m_member, false);
}

void GroupInviteMonitor::onGroupInviteAcknowledged(Protocol::Data::GroupInvite::InviteResponse inviteResponse)
{
    m_inviteResponse = inviteResponse;
    emit inviteMonitorDone(this, m_member, inviteResponse.accepted());
}
