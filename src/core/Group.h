#ifndef GROUP_H
#define GROUP_H

#include "ContactUser.h"
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
    Q_PROPERTY(int uniqueID READ uniqueID CONSTANT)

    friend class GroupsManager;
public:
    struct GroupInvite
    {
        QString message;
        QString author;
        QByteArray signature;
        QDateTime timestamp;
        QByteArray publicKey;
    };
    struct GroupMessage
    {
        QString message;
        QString author;
        QByteArray signature;
        QDateTime timestamp;
    };
    class GroupMember
    {
    public:
        GroupMember(ContactUser *contact) : m_isSelf(false), contact(contact) {}
        GroupMember(UserIdentity *identity) : m_isSelf(true), identity(identity) {}

        QString ricochetId() const { return (m_isSelf ? identity->contactID() : contact->contactID()); }
        CryptoKey key() const { return (m_isSelf ? identity->hiddenService()->privateKey() : contact->publicKey()); }
        bool isSelf() const { return m_isSelf; }

        QHash<int, Protocol::Channel*> channels() const { return (m_isSelf ? QHash<int, Protocol::Channel*>() : contact->connection()->channels()); }

        bool sendInvite(const Group::GroupInvite &invite);
        bool sendMessage(const Group::GroupMessage &message);

    private:
        bool m_isSelf;
        QByteArray m_position;
        QList<QByteArray> m_seeds;
        QList<QByteArray> m_seedHashes;
        union {
            UserIdentity *identity;
            ContactUser *contact;
        };
    };
    Group(int id, QObject *parent = 0);

    QString name() const;
    UserIdentity *selfIdentity() const { return m_selfIdentity; }
    QHash<QString, GroupMember*> groupMembers() const { return m_groupMembers; }

    void setName(const QString &name);
    void setSelfIdentity(UserIdentity *identity);

    int uniqueID() const { return m_uniqueID; }

    void addGroupMember(GroupMember *member);

    bool verifyPacket(const Protocol::Data::GroupInvite::Invite &packet);
    bool verifyPacket(const Protocol::Data::GroupChat::GroupMessage &packet);

    void testSendMessage();

signals:
    void nameChanged();
private slots:
    void onContactAdded(ContactUser *user);
    void onContactStatusChanged(ContactUser *user, int status);
    void onSettingsModified(const QString &key, const QJsonValue &value);
private:
    QHash<QString, GroupMember*> m_groupMembers;
    int m_uniqueID;
    QString m_name;
    UserIdentity *m_selfIdentity;
    QMap<QString, GroupMessage*> m_messageHistory;

    QByteArray signData(const QByteArray &data);

    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id);
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
