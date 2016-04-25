#include "GroupInviteChannel.h"
#include "Channel_p.h"
#include "core/GroupMember.h"
#include "core/UserIdentity.h"
#include "core/IdentityManager.h"
#include "tor/HiddenService.h"
#include <QDebug>

using namespace Protocol;

GroupInviteChannel::GroupInviteChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.group.invite"), direction, connection)
{

}

bool GroupInviteChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    return true;
}

bool GroupInviteChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    return true;
}

void GroupInviteChannel::receivePacket(const QByteArray &packet)
{
    Data::GroupInvite::Packet message;
    if (!message.ParseFromArray(packet.constData(), packet.size()))
    {
        closeChannel();
        return;
    }
    if (message.has_invite())
        handleInvite(message.invite());
    else if (message.has_invite_response())
        handleInviteResponse(message.invite_response());
    emit invitePacketReceived(message);
}

bool GroupInviteChannel::sendInvite(Protocol::Data::GroupInvite::Invite *invite)
{
    if (direction() != Outbound) {
        BUG() << "Group invite channels are unidirectional and this is not an outbound channel";
        return false;
    }
    QString text = QString::fromStdString(invite->message_text());
    if (text.isEmpty()) {
        qWarning() << "Invite message is empty, and it should've been discarded";
        return false;
    } else if (text.size() > MessageMaxCharacters) {
        qWarning() << "Invite message is too long (" << text.size() << "chars), and it shoudl've been limited already. Ignoring.";
        return false;
    }
    // Also converts to UTF-8
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite(invite);
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

void GroupInviteChannel::handleInvite(const Data::GroupInvite::Invite &invite)
{
    QScopedPointer<Data::GroupInvite::InviteResponse> inviteResponse(new Data::GroupInvite::InviteResponse);
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        inviteResponse->set_accepted(false);
    }
    if (!invite.has_author()
        || !invite.has_public_key()
        || !invite.has_signature()
        || !invite.has_timestamp()
        || !invite.has_message_text()) {
        qWarning() << "Rejected malformed invite packet";
        inviteResponse->set_accepted(false);
    }
    QString messageText = QString::fromStdString(invite.message_text());
    if (messageText.isEmpty() || messageText.size() > MessageMaxCharacters) {
        qWarning() << "Rejected invite with invalid message size: " << messageText.size();
        inviteResponse->set_accepted(false);
    }
    // find user who sent this packet
    ContactUser *user = identityManager->identities().at(0)->getContacts()->contactUserFromChannel(this); // assumes 1 identity
    if (!user) {
        BUG() << "No existing user found, so where did this packet come from?";
        return;
    }
    // verify the packet comtents
    CryptoKey userKey;
    userKey.loadFromData(QByteArray::fromStdString(invite.public_key()), CryptoKey::PublicKey, CryptoKey::DER);
    if (userKey.torServiceID() + QString::fromStdString(".onion") != user->hostname()) {
        qWarning() << "Rejected invite with public key that is not associated with the user it came from";
        inviteResponse->set_accepted(false);
    }
    if (user->publicKey().publicKeyDigest() == CryptoKey().publicKeyDigest()) {
        qInfo() << user->publicKey().publicKeyDigest() << CryptoKey().publicKeyDigest();
        qInfo() << "Setting user public key to public key in invite";
        user->setPublicKey(userKey);
    }
    QByteArray data = messageText.toUtf8() + QDateTime::fromMSecsSinceEpoch(invite.timestamp()).toUTC().toString().toUtf8();
    QByteArray sig = QByteArray::fromStdString(invite.signature());
    if (!userKey.verifyData(data, sig)) {
        qWarning() << "Rejected invite with a bad signature";
        inviteResponse->set_accepted(false);
    }
    // prepare response
    UserIdentity *identity = identityManager->identities().at(0); // assume 1 identity
    CryptoKey myKey = identity->hiddenService()->privateKey();
    QByteArray myKeyData = myKey.encodedPublicKey(CryptoKey::DER);
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    QString author = identity->contactID();
    if (!inviteResponse->accepted()) {
        messageText = QString::fromStdString("No thanks");
    } else {
        messageText = QString::fromStdString("Ok. Sure.");
    }
    sig = myKey.signData(messageText.toUtf8() + timestamp.toString().toUtf8());
    inviteResponse->set_message_text(messageText.toStdString());
    inviteResponse->set_signature(sig.constData(), sig.size());
    inviteResponse->set_author(author.toStdString());
    inviteResponse->set_timestamp(timestamp.toMSecsSinceEpoch());
    inviteResponse->set_public_key(myKeyData.constData(), myKeyData.size());
    // create group if all good
    if (inviteResponse->accepted()) {
        Group *group = groupsManager->createGroup(QString::fromStdString("New Group"));
        GroupMember *member = new GroupMember(user);
        group->addGroupMember(member);
    }
    // send response
    qDebug() << author;
    qDebug() << messageText;
    qDebug() << timestamp;
    qDebug() << sig;

    Protocol::Data::GroupInvite::Packet packet;
    packet.set_allocated_invite_response(inviteResponse.take());
    Channel::sendMessage(packet);
}

void GroupInviteChannel::handleInviteResponse(const Data::GroupInvite::InviteResponse &response)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound response on an oubout group invite channel";
        closeChannel();
        return;
    }
    if (!response.has_author()
        || !response.has_message_text()
        || !response.has_public_key()
        || !response.has_signature()
        || !response.has_timestamp()) {
        qWarning() << "Rejected malformed invite response";
        return;
    }
    Group *group = groupsManager->groupFromChannel(this);
    if (!group) {
        BUG() << "Group not found for this invite channel";
        return;
    }
    ContactUser *user = group->selfIdentity()->contacts.contactUserFromChannel(this);
    if (!user) {
        BUG() << "Invite response from unknown user. Where did this packet come from?";
        return;
    }
    if (user->publicKey().publicKeyDigest() == CryptoKey().publicKeyDigest()) {
        QByteArray keyData = QByteArray::fromStdString(response.public_key());
        CryptoKey key;
        if (key.loadFromData(keyData, CryptoKey::PublicKey, CryptoKey::DER)) {
            user->setPublicKey(key);
        } else {
            qDebug() << "Failed to load public key from data in invite response";
        }
        qDebug() << "keyData" << keyData;
    }
    if (!group->verifyPacket(response)) {
        qWarning() << "Rejected invite response that did not verify";
        closeChannel();
        return;
    }
    if (response.accepted()) {
        qDebug() << "GroupInviteChannel::handleInviteResponse accepted";
        //group->beginProtocolIntroduction(response);
    } else {
        qDebug() << "GroupInviteChannel::handleInviteResponse denied";
    }
    emit inviteAcknowleged(response.accepted());
    closeChannel();
}
