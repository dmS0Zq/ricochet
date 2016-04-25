#include "GroupMember.h"

#include "protocol/GroupInviteChannel.h"
#include "protocol/GroupChatChannel.h"

GroupMember::GroupMember(ContactUser *contact) : m_isSelf(false), contact(contact)
{
    connectSignals();
}

GroupMember::GroupMember(UserIdentity *identity) : m_isSelf(true), identity(identity)
{
}

bool GroupMember::sendInvite(Protocol::Data::GroupInvite::Invite *invite)
{
    if (this->m_isSelf) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupInviteChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectSignals();
    }
    if (!channel->sendInvite(invite)) {
        return false;
    }
    return true;
}

bool GroupMember::sendMessage(Protocol::Data::GroupChat::GroupMessage message)
{
    if (this->m_isSelf) return true;
    auto channel = contact->connection()->findChannel<Protocol::GroupChatChannel>(Protocol::Channel::Outbound);
    if (!channel) {
        channel = new Protocol::GroupChatChannel(Protocol::Channel::Outbound, contact->connection().data());
        if (!channel->openChannel()) {
            return false;
        }
        connectSignals();
    }
    if (!channel->sendGroupMessage(message)) {
        return false;
    }
    return true;
}

void GroupMember::connectSignals()
{
    Protocol::GroupInviteChannel *inviteChannel = contact->connection()->findChannel<Protocol::GroupInviteChannel>(Protocol::Channel::Outbound);
    if (inviteChannel) {
        connect(inviteChannel, &Protocol::GroupInviteChannel::inviteAcknowleged, [this](bool accepted) {emit groupInviteAcknowledged(accepted);});
    }
    Protocol::GroupChatChannel *chatChannel = contact->connection()->findChannel<Protocol::GroupChatChannel>(Protocol::Channel::Outbound);
    if (chatChannel) {
        connect(chatChannel, &Protocol::GroupChatChannel::messageAcknowledged, [this](Protocol::Data::GroupChat::GroupMessage message, bool accepted) {emit groupMessageAcknowledged(message, this, accepted);});
    }
}
