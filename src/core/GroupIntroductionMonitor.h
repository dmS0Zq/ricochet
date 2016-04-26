#ifndef GROUPINTRODUCTIONMONITOR_H
#define GROUPINTRODUCTIONMONITOR_H

#include "core/GroupMember.h"
#include "protocol/GroupMetaChannel.pb.h"

#include <QObject>
#include <QTimer>
#include <QList>

class GroupIntroductionMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupIntroductionMonitor)

public:
    GroupIntroductionMonitor(Protocol::Data::GroupMeta::Introduction introduction, QHash<QString, GroupMember*> members, QObject *parent = 0);

    Protocol::Data::GroupMeta::Introduction introduction() const { return m_introduction; }
signals:
    void introductionMonitorDone(GroupIntroductionMonitor *monitor, bool totalAcceptance);
public slots:
    void onIntroductionResponseReceived(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse);
private slots:
    void onTimeout();
private:
    Protocol::Data::GroupMeta::Introduction m_introduction;
    QHash<QString, GroupMember*> m_outstandingMembers;
};

#endif // GROUPINTRODUCTIONMONITOR_H
