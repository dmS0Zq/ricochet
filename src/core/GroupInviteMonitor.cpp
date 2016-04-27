#include "GroupInviteMonitor.h"

#include <QDebug>
#include <QTimer>

using Protocol::Data::GroupInvite::Invite;
using Protocol::Data::GroupInvite::InviteResponse;

GroupInviteMonitor::GroupInviteMonitor(Invite &invite, GroupMember *invitee, GroupMember *inviter, Group *group, QObject *parent)
    : QObject(parent)
    , m_invite(invite)
    , m_invitee(invitee)
    , m_inviter(inviter)
    , m_group(group)
{
    m_timer.setSingleShot(true);
    if (m_invitee->isSelf()) {
        m_timer.setInterval(10 * 1000);  
    } else {
        m_timer.setInterval(10 * 1000);
    }
}

bool GroupInviteMonitor::go()
{
    if (!m_group->state() == Group::State::PendingInvite) {
        qWarning() << "GroupInviteMonitor::go refusing to start send invite in because group in state" << m_group->state();
        return false;
    }
    if (!Group::verifyPacket(m_invite)) {
        qWarning() << "GroupInviteMonitor::go refusing to start send invite which didn't verify";
        return false;
    }
    connect(this, &GroupInviteMonitor::inviteMonitorDone, m_group, &Group::onInviteMonitorDone);
    connect(m_invitee, &GroupMember::inviteResponseReceived, this, &GroupInviteMonitor::onInviteResponseReceived);
    connect(&m_timer, &QTimer::timeout, this, &GroupInviteMonitor::onTimeout);
    if (!m_invitee->sendInvite(m_invite)) {
        qWarning() << "GroupInviteMonitor::go could not send invite to invitee";
        return false;
    }
    m_timer.start();
    return true;
}

void GroupInviteMonitor::onTimeout()
{
}

void GroupInviteMonitor::onInviteResponseReceived(const InviteResponse &response)
{
    m_response = response;
    m_timer.stop();
    emit inviteMonitorDone(this, response.accepted());
}
