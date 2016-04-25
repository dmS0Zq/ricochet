#ifndef GROUPMESSAGEMONITOR_H
#define GROUPMESSAGEMONITOR_H

#include "core/GroupMember.h"
#include "protocol/GroupChatChannel.pb.h"

#include <QObject>
#include <QTimer>
#include <QList>

class GroupMessageMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupMessageMonitor)

public:
    GroupMessageMonitor(Protocol::Data::GroupChat::GroupMessage message, QHash<QString, GroupMember*> members, QObject *parent = 0);

    Protocol::Data::GroupChat::GroupMessage message() const { return m_message; }
signals:
    void messageMonitorDone(GroupMessageMonitor *monitor, bool totalAcknowledgement);
public slots:
    void onGroupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted);
private slots:
    void onTimeout();
private:
    Protocol::Data::GroupChat::GroupMessage m_message;
    QHash<QString, GroupMember*> m_outstandingMembers;
    QTimer *m_timer;
};

#endif // GROUPMESSAGE_MONITOR_H
