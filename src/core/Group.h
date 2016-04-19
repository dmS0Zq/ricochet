#ifndef GROUP_H
#define GROUP_H

#include "ContactUser.h"
#include "protocol/GroupChatChannel.pb.h"
#include "protocol/GroupInviteChannel.pb.h"
#include "utils/CryptoKey.h"
#include <QObject>
#include <QHash>

class Group : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Group)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int uniqueID READ uniqueID CONSTANT)

    friend class GroupsManager;
public:
    enum State
    {
        Ready,
        InvitationAcceptance,
        Rebalance,
        Unknown
    };
    struct GroupMember
    {
        QString ricochetId;
        CryptoKey key;
        QList<QByteArray> seeds;
        QList<QByteArray> seedHashes;
        QByteArray position;
        QSharedPointer<Protocol::Connection> connection;
    };
    Group(int id, QObject *parent = 0);
    QList<QByteArray> seeds() const { return m_seeds; }
    QList<QByteArray> seedHashes() const;
    QString name() const;
    UserIdentity *selfIdentity() const { return m_selfIdentity; }
    void setName(const QString &name);
    void setSelfIdentity(UserIdentity *identity);
    int uniqueID() const { return m_uniqueID; }
    bool verifyMessage(const Protocol::Data::GroupChat::GroupMessage &message);
    void addGroupMember(GroupMember *member);
    bool verifyPacket(const Protocol::Data::GroupInvite::Invite &packet);
signals:
    void nameChanged();
    void stateChanged(State oldState, State newState);
private slots:
    void onContactAdded(ContactUser *user);
    void onContactStatusChanged(ContactUser *user, int status);
    void onSettingsModified(const QString &key, const QJsonValue &value);
private:
    QHash<QString, GroupMember*> m_groupMembers;
    QList<QByteArray> m_seeds;
    int m_uniqueID;
    Group::State m_state;
    QString m_name;
    UserIdentity *m_selfIdentity;

    QByteArray signData(const QByteArray &data);

    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id);
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
