#ifndef GROUPMEMBER_H
#define GROUPMEMBER_H

#include "core/UserIdentity.h"
#include "protocol/GroupChatChannel.pb.h"
#include "protocol/GroupInviteChannel.pb.h"
#include "protocol/GroupMetaChannel.pb.h"
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
    bool setKey(QByteArray publicKey);

    QHash<int, Protocol::Channel*> channels() const { return (m_isSelf ? QHash<int, Protocol::Channel*>() : contact->connection()->channels()); }
    const Protocol::Connection *connection() { return (m_isSelf ? nullptr : contact->connection().data()); }

    bool sendInvite(Protocol::Data::GroupInvite::Invite invite);
    bool sendInviteResponse(Protocol::Data::GroupInvite::InviteResponse response);
    bool sendIntroductionAccepted(Protocol::Data::GroupInvite::IntroductionAccepted &accepted);
    bool sendMessage(Protocol::Data::GroupChat::GroupMessage message);
    bool sendIntroduction(Protocol::Data::GroupMeta::Introduction introduction);
    bool sendIntroductionResponse(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse);

    // connects signals from incoming/outgoing channels respectively
    void connectIncomingSignals();
    void connectOutgoingSignals();
signals:
    void groupMessageReceived(Protocol::Data::GroupChat::GroupMessage &message);
    void groupIntroductionReceived(Protocol::Data::GroupMeta::Introduction &introduction);

    void inviteResponseReceived(const Protocol::Data::GroupInvite::InviteResponse &response);
    void groupMessageAcknowledged(Protocol::Data::GroupChat::GroupMessage message, GroupMember *member, bool accepted);
    void groupIntroductionResponseReceived(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse);

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
#endif // GROUPMEMBER_H
