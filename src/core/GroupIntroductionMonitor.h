#ifndef GROUPINTRODUCTIONMONITOR_H
#define GROUPINTRODUCTIONMONITOR_H

#include "core/GroupMember.h"
#include "protocol/GroupMetaChannel.pb.h"

#include <QObject>
#include <QTimer>
#include <QList>

class Group;

class GroupIntroductionMonitor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupIntroductionMonitor)

public:
    GroupIntroductionMonitor(Protocol::Data::GroupMeta::Introduction introduction, GroupMember *invitee, Group *group, QHash<QString, GroupMember*> members, QObject *parent = 0);

    Protocol::Data::GroupMeta::Introduction introduction() const { return m_introduction; }
    Protocol::Data::GroupInvite::InviteResponse inviteResponse() const { return m_introduction.invite_response(); }
    QHash<QString, QByteArray> introductionResponseSignatures() const { return m_introductionResponseSignatures; }
    bool go();
signals:
    void introductionMonitorDone(GroupIntroductionMonitor *monitor, GroupMember *invitee, bool totalAcceptance);
public slots:
    void onIntroductionResponseReceived(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse);
private slots:
    void onTimeout();
private:
    Protocol::Data::GroupMeta::Introduction m_introduction;
    GroupMember *m_invitee;
    Group *m_group;
    QHash<QString, GroupMember*> m_outstandingMembers;
    QHash<QString, QByteArray> m_introductionResponseSignatures;
    QTimer m_timer;
};

#endif // GROUPINTRODUCTIONMONITOR_H
