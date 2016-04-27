#include "GroupIntroductionMonitor.h"

#include "core/Group.h"
#include <QDebug>
#include <QTimer>

using Protocol::Data::GroupMeta::Introduction;

GroupIntroductionMonitor::GroupIntroductionMonitor(Introduction introduction, GroupMember *invitee, Group *group, QHash<QString, GroupMember*> members, QObject *parent)
    : QObject(parent)
    , m_introduction(introduction)
    , m_invitee(invitee)
    , m_group(group)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(10 * 1000);
    foreach (auto member, members) {
        if (!member->isSelf() && member->ricochetId() != m_invitee->ricochetId()) {
            m_outstandingMembers.insert(member->ricochetId(), member);
        }
    }
}

void GroupIntroductionMonitor::onTimeout()
{
    emit introductionMonitorDone(this, m_invitee, (m_outstandingMembers.size() < 1));
}

void GroupIntroductionMonitor::onIntroductionResponseReceived(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse)
{
    if (!introductionResponse.accepted()) {
        return;
    }
    if (m_outstandingMembers.contains(QString::fromStdString(introductionResponse.author()))) {
        m_outstandingMembers.remove(QString::fromStdString(introductionResponse.author()));
        m_introductionResponseSignatures.insert(QString::fromStdString(introductionResponse.author()), QByteArray::fromStdString(introductionResponse.signature()));
    }
    if (m_outstandingMembers.size() < 1) {
        emit introductionMonitorDone(this, m_invitee, true);
    }
}

bool GroupIntroductionMonitor::go()
{
    connect(this, &GroupIntroductionMonitor::introductionMonitorDone, m_group, &Group::onIntroductionMonitorDone);
    if (m_outstandingMembers.size() < 1) {
        emit introductionMonitorDone(this, m_invitee, true);
        return true;
    }
    connect(&m_timer, &QTimer::timeout, this, &GroupIntroductionMonitor::onTimeout);
    foreach (auto member, m_outstandingMembers) {
        if (!member->isSelf() && member->ricochetId() != m_invitee->ricochetId()) {
            connect(member, &GroupMember::groupIntroductionResponseReceived, this, &GroupIntroductionMonitor::onIntroductionResponseReceived);
            if (!member->sendIntroduction(m_introduction)) {
                return false;
            }
        }
    }
    m_timer.start();
    return true;
}
