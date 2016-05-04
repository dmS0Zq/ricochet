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

class GroupInviteMonitor;

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
    State state() { return m_state; }
    QHash<QString, GroupMember*> groupMembers() { return m_groupMembers; }

    GroupMember *groupMemberFromRicochetId(QString ricochetId);

    void setName(const QString &name);
    void setState(const State &state);

    int uniqueID() { return m_uniqueID; }

    void addGroupMember(GroupMember *member);
    void removeGroupMember(GroupMember *member);

    static bool verifyPacket(const Protocol::Data::GroupInvite::Invite &packet);
    static bool verifyPacket(const Protocol::Data::GroupInvite::InviteResponse &packet);
    static bool verifyPacket(const Protocol::Data::GroupInvite::IntroductionAccepted &packet);
    static bool verifyPacket(const Protocol::Data::GroupMeta::Introduction &packet);
    static bool verifyPacket(const Protocol::Data::GroupMeta::IntroductionResponse &packet);
    static bool verifyPacket(const Protocol::Data::GroupMeta::Leave &packet);
    static bool verifyPacket(const Protocol::Data::GroupChat::GroupMessage &packet);
    static bool verifyPacket(const Protocol::Data::GroupChat::GroupMessageAcknowledge &packet);

    static UserIdentity *selfIdentity();
    static QByteArray signData(const QByteArray &data);

    Q_INVOKABLE void beginProtocolInvite(ContactUser *contact);
    Q_INVOKABLE void beginProtocolSendMessage(QString messageText);
    Q_INVOKABLE void leaveGroup();
signals:
    void nameChanged();
    void stateChanged(State state);
    void groupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted);
public slots:
    void onInviteMonitorDone(GroupInviteMonitor *monitor, bool responseReceived);
    void onIntroductionMonitorDone(GroupIntroductionMonitor *monitor, GroupMember *invitee, bool totalAcceptance);
private slots:
    void onChannelOpen(Protocol::Channel *channel);
    void onSettingsModified(const QString &key, const QJsonValue &value);
    void onGroupMessageReceived(Protocol::Data::GroupChat::GroupMessage &message);
    void onGroupIntroductionReceived(Protocol::Data::GroupMeta::Introduction &introduction);
    void onLeaveReceived(Protocol::Data::GroupMeta::Leave leave);
    void onMessageMonitorDone(GroupMessageMonitor *monitor, bool totalAcknowlegement);
private:
    QHash<QString, GroupMember*> m_groupMembers;
    int m_uniqueID;
    QString m_name;
    GroupMessageHistory m_messageHistory;
    QList<GroupMessageMonitor*> m_messageMonitors;
    State m_state;


    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id, QObject *parent);

    void beginProtocolIntroduction(const Protocol::Data::GroupInvite::InviteResponse &response, GroupMember *invitee);
    void beginProtocolForwardMessage(Protocol::Data::GroupChat::GroupMessage &message);
    void beginProtocolRebalance();
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
