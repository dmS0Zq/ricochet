#ifndef GROUPMEMBER_H
#define GROUPMEMBER_H

#include "core/UserIdentity.h"
#include "protocol/GroupChatChannel.pb.h"
#include "protocol/GroupInviteChannel.pb.h"
#include "tor/HiddenService.h"
#include "utils/CryptoKey.h"

#include <QObject>

class GroupMember : public QObject
{
    Q_OBJECT

public:
    GroupMember(ContactUser *contact);
    GroupMember(UserIdentity *identity);

    QString nickname() const  { return (m_isSelf ? QString::fromStdString("Me") : contact->nickname()); }
    QString ricochetId() const { return (m_isSelf ? identity->contactID() : contact->contactID()); }
    CryptoKey key() const { return (m_isSelf ? identity->hiddenService()->privateKey() : contact->publicKey()); }
    bool isSelf() const { return m_isSelf; }

    QHash<int, Protocol::Channel*> channels() const { return (m_isSelf ? QHash<int, Protocol::Channel*>() : contact->connection()->channels()); }

    bool sendInvite(Protocol::Data::GroupInvite::Invite *invite);
    bool sendMessage(Protocol::Data::GroupChat::GroupMessage message);

signals:
    void groupInviteAcknowledged(bool accepted);
    void groupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted);

private:
    bool m_isSelf;
    QByteArray m_position;
    QList<QByteArray> m_seeds;
    QList<QByteArray> m_seedHashes;
    union {
        UserIdentity *identity;
        ContactUser *contact;
    };
    void connectSignals();
};
#endif // GROUPMEMBER_H
