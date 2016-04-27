#include "GroupMember.h"

#include "protocol/GroupInviteChannel.h"
#include "protocol/GroupChatChannel.h"
#include "protocol/GroupMetaChannel.h"
#include "utils/Useful.h"

GroupMember::GroupMember(ContactUser *contact) : m_isSelf(false), contact(contact)
{
    connectOutgoingSignals();
}

GroupMember::GroupMember(UserIdentity *identity) : m_isSelf(true), identity(identity)
{
}

bool GroupMember::setKey(QByteArray publicKey)
{
    if (this->isSelf()) return true;
    CryptoKey oldKey = contact->publicKey();
    CryptoKey newKey;
    newKey.loadFromData(publicKey, CryptoKey::PublicKey, CryptoKey::DER);
    if (oldKey.publicKeyDigest() == QByteArray() && newKey.torServiceID()+QString::fromStdString(".onion") == contact->hostname()) {
        contact->setPublicKey(newKey);
        return true;
    }
    if (oldKey.torServiceID()+QString::fromStdString(".onion") == contact->hostname()) return true;
    return false;
}

bool GroupMember::sendInvite(Protocol::Data::GroupInvite::Invite invite)
{
    if (this->isSelf()) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupInviteChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectOutgoingSignals();
    }
    if (!channel->sendInvite(invite)) {
        return false;
    }
    return true;
}

bool GroupMember::sendInviteResponse(Protocol::Data::GroupInvite::InviteResponse response)
{
    if (this->isSelf()) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Inbound);
    if (!channel) {
        BUG() << "Trying to send an invite response, but no inbound GroupInviteChannel exists";
        return false;
    }
    if (!channel->sendInviteResponse(response)) {
        return false;
    }
    return true;
}

bool GroupMember::sendIntroductionAccepted(Protocol::Data::GroupInvite::IntroductionAccepted &accepted)
{
    if (this->isSelf()) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupInviteChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectOutgoingSignals();
    }
    if (!channel->sendIntroductionAccepted(accepted)) {
        return false;
    }
    return true;
}

bool GroupMember::sendMessage(Protocol::Data::GroupChat::GroupMessage message)
{
    if (this->isSelf()) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupChatChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupChatChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectOutgoingSignals();
    }
    if (!channel->sendGroupMessage(message)) {
        return false;
    }
    return true;
}

bool GroupMember::sendIntroduction(Protocol::Data::GroupMeta::Introduction introduction)
{
    if (this->isSelf()) return true;
    if (introduction.invite_response().author() == contact->contactID().toStdString()) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupMetaChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupMetaChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectOutgoingSignals();
    }
    if (!channel->sendIntroduction(introduction)) {
        return false;
    }
    return true;
}

bool GroupMember::sendIntroductionResponse(Protocol::Data::GroupMeta::IntroductionResponse introductionResponse)
{
   if (this->isSelf()) return true;
   auto channel = contact->connection()->findChannel<Protocol::GroupMetaChannel>(Protocol::Channel::Outbound);
   if (!channel) {
       channel = new Protocol::GroupMetaChannel(Protocol::Channel::Outbound, contact->connection().data());
       if (!channel->openChannel()) {
           return false;
       }
       connectOutgoingSignals();
   }
   if (!channel->sendIntroductionResponse(introductionResponse)) {
       return false;
   }
   return true;
}

void GroupMember::connectIncomingSignals()
{
    Protocol::GroupInviteChannel *inviteChannel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Inbound);
    if (inviteChannel) {
    }
    Protocol::GroupChatChannel *chatChannel = contact->connection()->findChannel<Protocol::GroupChatChannel>(Protocol::Channel::Inbound);
    if (chatChannel) {
        connect(chatChannel, &Protocol::GroupChatChannel::groupMessageReceived, [this](Protocol::Data::GroupChat::GroupMessage message) {emit groupMessageReceived(message);});
    }
    Protocol::GroupMetaChannel *metaChannel = contact->connection()->findChannel<Protocol::GroupMetaChannel>(Protocol::Channel::Inbound);
    if (metaChannel) {
        connect(metaChannel, &Protocol::GroupMetaChannel::introductionReceived, [this](Protocol::Data::GroupMeta::Introduction introduction) {emit groupIntroductionReceived(introduction);});
        connect(metaChannel, &Protocol::GroupMetaChannel::introductionResponseReceived, [this](Protocol::Data::GroupMeta::IntroductionResponse introductionResponse) { emit groupIntroductionResponseReceived(introductionResponse);});
    }
}

void GroupMember::connectOutgoingSignals()
{
    Protocol::GroupInviteChannel *inviteChannel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Outbound);
    if (inviteChannel) {
        connect(inviteChannel, &Protocol::GroupInviteChannel::inviteResponseReceived, [this](const Protocol::Data::GroupInvite::InviteResponse &response) {emit inviteResponseReceived(response);});
    }
    Protocol::GroupChatChannel *chatChannel = contact->connection()->findChannel<Protocol::GroupChatChannel>(Protocol::Channel::Outbound);
    if (chatChannel) {
        connect(chatChannel, &Protocol::GroupChatChannel::messageAcknowledged, [this](Protocol::Data::GroupChat::GroupMessage message, bool accepted) {emit groupMessageAcknowledged(message, this, accepted);});
    }
    Protocol::GroupMetaChannel *metaChannel = contact->connection()->findChannel<Protocol::GroupMetaChannel>(Protocol::Channel::Outbound);
    if (metaChannel) {
        connect(metaChannel, &Protocol::GroupMetaChannel::introductionResponseReceived, [this](Protocol::Data::GroupMeta::IntroductionResponse introductionResponse) {emit groupIntroductionResponseReceived(introductionResponse);});
    }
}
