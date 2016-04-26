#ifndef GROUP_H
#define GROUP_H

#include "core/ContactUser.h"
#include "core/GroupIntroductionMonitor.h"
#include "core/GroupInviteMonitor.h"
#include "core/GroupMember.h"
#include "core/GroupMessageMonitor.h"
#include "core/GroupMessageHistory.h"
#include "core/UserIdentity.h"
#include "protocol/GroupChatChannel.pb.h"
#include "protocol/GroupInviteChannel.pb.h"
#include "protocol/Channel.h"
#include "tor/HiddenService.h"
#include "utils/CryptoKey.h"
#include <QObject>
#include <QHash>
#include <QMap>

class Group : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Group)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(State state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(int uniqueID READ uniqueID CONSTANT)

    friend class GroupsManager;
public:
    enum State {
        Undefined = 0,
        Good,
        PendingInvite,
        PendingIntroduction,
        Rebalancing,
        IssueResolution
    };
    Group(int id, QObject *parent = 0);

    QString name() const { return m_name; }
    State state() const { return m_state; }
    QHash<QString, GroupMember*> groupMembers() const { return m_groupMembers; }

    void setName(const QString &name);
    void setState(const State &state);

    int uniqueID() const { return m_uniqueID; }

    void addGroupMember(GroupMember *member);
    void removeGroupMember(GroupMember *member);

    bool verifyPacket(const Protocol::Data::GroupInvite::Invite &packet);
    bool verifyPacket(const Protocol::Data::GroupInvite::InviteResponse &packet);
    bool verifyPacket(const Protocol::Data::GroupChat::GroupMessage &packet);
    bool verifyPacket(const Protocol::Data::GroupMeta::IntroductionResponse &packet);

    void testSendMessage();
    void sendMessage(Protocol::Data::GroupChat::GroupMessage message);
    void sendInvite(Protocol::Data::GroupInvite::Invite invite, GroupMember *member);
    void sendIntroduction(Protocol::Data::GroupMeta::Introduction introduction);

    UserIdentity *selfIdentity() const;

    Q_INVOKABLE void beginProtocolInvite(ContactUser *contact);
signals:
    void nameChanged();
    void stateChanged(State state);
    void groupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted);
private slots:
    void onContactAdded(ContactUser *user);
    void onContactStatusChanged(ContactUser *user, int status);
    void onSettingsModified(const QString &key, const QJsonValue &value);
    void onMessageMonitorDone(GroupMessageMonitor *monitor, bool totalAcknowlegement);
    void onInviteMonitorDone(GroupInviteMonitor *monitor, GroupMember *member, bool responseReceived);
    void onIntroductionMonitorDone(GroupIntroductionMonitor *monitor, bool totalAcceptance);
private:
    QHash<QString, GroupMember*> m_groupMembers;
    int m_uniqueID;
    QString m_name;
    GroupMessageHistory m_messageHistory;
    QList<GroupMessageMonitor*> m_messageMonitors;
    State m_state;

    QByteArray signData(const QByteArray &data);

    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id, QObject *parent);

    void beginProtocolIntroduction(Protocol::Data::GroupInvite::InviteResponse inviteResponse);
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
