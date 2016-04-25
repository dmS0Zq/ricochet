#include "GroupMessageMonitor.h"

#include <QDebug>
#include <QTimer>

using Protocol::Data::GroupChat::GroupMessage;

GroupMessageMonitor::GroupMessageMonitor(GroupMessage message, QHash<QString, GroupMember*> members, QObject *parent)
    : QObject(parent)
{
    foreach (auto member, members)
        if (!member->isSelf())
            m_outstandingMembers.insert(member->ricochetId(), member);
    m_message = message;
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(10 * 1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_timer->start();
}

void GroupMessageMonitor::onTimeout()
{
    emit messageMonitorDone( this, (m_outstandingMembers.size() < 1) );
}

void GroupMessageMonitor::onGroupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted)
{
    if (!accepted)
        return;
    if (m_outstandingMembers.contains(member->ricochetId())) {
        m_outstandingMembers.remove(member->ricochetId());
    }
    if (m_outstandingMembers.size() < 1) {
        m_timer->stop();
        emit messageMonitorDone(this, true);
    }
}
