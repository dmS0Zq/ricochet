#include "GroupInviteChannel.h"
#include "Channel_p.h"
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

bool GroupInviteChannel::sendInvite(QString text, QDateTime time, QByteArray signature, QString author, QByteArray publicKey)
{
    if (direction() != Outbound) {
        BUG() << "Group invite channels are unidirectional and this is not an outbound channel";
        return false;
    }
    QScopedPointer<Data::GroupInvite::Invite> message(new Data::GroupInvite::Invite);
    if (text.isEmpty()) {
        BUG() << "Invite message is empty, and it should've been discarded";
        return false;
    } else if (text.size() > MessageMaxCharacters) {
        BUG() << "Invite message is too long (" << text.size() << "chars), and it shoudl've been limited already. Truncated.";
        text.truncate(MessageMaxCharacters);
    }
    // Also converts to UTF-8
    message->set_message_text(text.toStdString());
    message->set_author(author.toStdString());
    if (!time.isNull())
        message->set_timestamp(time.toMSecsSinceEpoch());
    message->set_signature(signature.constData(), signature.size());
    message->set_public_key(publicKey.constData(), publicKey.size());
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite(message.take());
    if (!Channel::sendMessage(packet))
        return false;
    return true;
}

void GroupInviteChannel::handleInvite(const Data::GroupInvite::Invite &invite)
{
    QScopedPointer<Data::GroupInvite::InviteResponse> response(new Data::GroupInvite::InviteResponse);
    QString text = QString::fromStdString(invite.message_text());
    CryptoKey publicKey;
    bool loadedKey = publicKey.loadFromData(QByteArray::fromStdString(invite.public_key()), CryptoKey::PublicKey, CryptoKey::DER);
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on an outbound group invite channel";
        response->set_accepted(false);
    } else if (text.isEmpty()) {
        qWarning() << "Rejected empty group invite message";
        response->set_accepted(false);
    } else if (text.size() > MessageMaxCharacters) {
        qWarning() << "Rejected oversize group invite message of " << text.size() << "chars";
        response->set_accepted(false);
    } else if (!loadedKey) {
        qWarning() << "Rejected message without valid public key";
        response->set_accepted(false);
    } else {
        response->set_accepted(true);
    }
    // create group
    Group *group = groupsManager->createGroup(QString::fromStdString("New Group"));
    // find user who sent this packet
    ContactUser *user = group->selfIdentity()->getContacts()->contactUserFromChannel(this);
    if (!user) {
        BUG() << "No existing user found, so where did this packet come from?";
        return;
    }
    user->setPublicKey(publicKey);
    // makes sure that user is the one who sent this message
    if (user->contactID() != QString::fromStdString(invite.author())) {
        qDebug() << "GroupInviteChannel::handleInvite:" << QString::fromStdString(invite.author()) << "!=" << user->contactID() << "which is where it came from";
        return;
    }
    // add other user to group
    Group::GroupMember *member = new Group::GroupMember();
    member->connection = QSharedPointer<Protocol::Connection>(this->connection());
    member->key = user->publicKey();
    member->ricochetId = user->contactID();
    group->addGroupMember(member);
    // verify that the signature on the pcaket is good
    if (!group->verifyPacket(invite)) {
        qDebug() << "GroupInviteChannel::handleInvite: packet did not verify";
        return;
    }
    QByteArray myPublicKey;
    UserIdentity *self = identityManager->identities().at(0); // assume 1 identity
    myPublicKey = self->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
    response->set_public_key(myPublicKey.constData(), myPublicKey.size());
    response->set_author(self->contactID().toStdString());
    Data::GroupInvite::Packet packet;
    packet.set_allocated_invite_response(response.take());
    Channel::sendMessage(packet);
}

void GroupInviteChannel::handleInviteResponse(const Data::GroupInvite::InviteResponse &response)
{
    if (direction() != Outbound) {
        qWarning() << "Rejected inbound response on an oubout group invite channel";
        closeChannel();
        return;
    }
    if (response.accepted()) {
        qDebug() << "GroupInviteChannel::handleInviteResponse accepted";
    } else {
        qDebug() << "GroupInviteChannel::handleInviteResponse denied";
    }
    qDebug() << "GroupInviteChannel::handleInviteResponse got" << QString::fromStdString(response.author()) << QString::fromStdString(response.public_key()) << response.timestamp();
    ContactUser *user = identityManager->identities().at(0)->getContacts()->contactUserFromChannel(this); // assume 1 identity
    CryptoKey publicKey;
    publicKey.loadFromData(QByteArray::fromStdString(response.public_key()), CryptoKey::PublicKey, CryptoKey::DER);
    user->setPublicKey(publicKey);
    closeChannel();

}
