#include "Group.h"
#include "core/IdentityManager.h"
#include "protocol/ControlChannel.h"
#include "protocol/GroupChatChannel.h"
#include "protocol/GroupInviteChannel.h"
#include "tor/HiddenService.h"
#include "utils/SecureRNG.h"
#include "utils/Useful.h"
#include <QCryptographicHash>
#include <QDebug>

Group::Group(int id, QObject *parent)
    : QObject(parent)
    , m_uniqueID(id)
    , m_state(State::Undefined)
{
    Q_ASSERT(m_uniqueID >= 0);
}

void Group::onSettingsModified(const QString &key, const QJsonValue &value)
{
    Q_UNUSED(value);
    if (key == QLatin1String("name"))
        emit nameChanged();
}

void Group::setName(const QString &name)
{
    m_name = name;
    emit nameChanged();
}

void Group::setState(const State &state)
{
    m_state = state;
    emit stateChanged(m_state);
}

void Group::addGroupMember(GroupMember *member)
{
    if (!m_groupMembers.contains(member->ricochetId()))
    {
        qDebug() << "Adding" << member->nickname() << "to group";
        m_groupMembers[member->ricochetId()] = member;
        connect(member->connection().data(), &Protocol::Connection::channelOpened, this, &Group::onChannelOpen);
        connect(member, &GroupMember::groupMessageReceived, this, &Group::onGroupMessageReceived);
        connect(member, &GroupMember::inviteReceived, this, &Group::onInviteReceived);
    }
    else {
        qDebug() << member->ricochetId() << "already in group";
    }
}

QByteArray Group::signData(const QByteArray &data)
{
    return m_groupMembers[selfIdentity()->contactID()]->key().signData(data);
}

Group *Group::addNewGroup(int id, QObject *parent)
{
    Group *group = new Group(id, parent);
    group->setName(QString());
    return group;
}

bool Group::verifyPacket(const Protocol::Data::GroupInvite::Invite &packet)
{
    if (!packet.has_message_text() || !packet.has_signature() || !packet.has_timestamp() || !packet.has_author())
        return false;
    QString text = QString::fromStdString(packet.message_text());
    QString time = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString();
    QByteArray data = QByteArray::fromStdString(text.toStdString() + time.toStdString());
    QString author = QString::fromStdString(packet.author());
    QByteArray sig = QByteArray::fromStdString(packet.signature());
    return m_groupMembers[author]->key().verifyData(data, sig);
}

bool Group::verifyPacket(const Protocol::Data::GroupInvite::InviteResponse &packet) {
    if (!packet.has_author() || !packet.has_message_text() || !packet.has_signature() || !packet.has_timestamp())
        return false;
    QString text = QString::fromStdString(packet.message_text());
    QString time = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString();
    QByteArray data = QByteArray::fromStdString(text.toStdString() + time.toStdString());
    QString author = QString::fromStdString(packet.author());
    QByteArray sig = QByteArray::fromStdString(packet.signature());
    return m_groupMembers[author]->key().verifyData(data, sig);
}

bool Group::verifyPacket(const Protocol::Data::GroupMeta::IntroductionResponse &packet)
{
    if (!packet.has_author() || !packet.has_message_id() || !packet.has_signature() || !packet.has_timestamp())
        return false;
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupChat::GroupMessage &packet)
{
    if (!packet.has_author() || !packet.has_message_text() || !packet.has_signature() || !packet.has_timestamp())
        return false;
    QString text = QString::fromStdString(packet.message_text());
    QString time = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString();
    QByteArray data = QByteArray::fromStdString(text.toStdString() + time.toStdString());
    QString author = QString::fromStdString(packet.author());
    QByteArray sig = QByteArray::fromStdString(packet.signature());
    return m_groupMembers[author]->key().verifyData(data, sig);
}

UserIdentity *Group::selfIdentity() const
{
    return identityManager->identities().at(0); // assume 1 identity
}

void Group::beginProtocolSendMessage(QString messageText)
{
    if (m_state == State::Undefined || m_state == State::Rebalancing || m_state == State::IssueResolution) {
        qWarning() << "Not sending message because in state" << m_state;
        return;
    }
    Protocol::Data::GroupChat::GroupMessage message;
    if (messageText.size() > Protocol::GroupChatChannel::MessageMaxCharacters)
        messageText.truncate(Protocol::GroupChatChannel::MessageMaxCharacters);
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    message.set_message_id(SecureRNG::randomInt(UINT_MAX));
    message.set_timestamp(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    message.set_author(selfIdentity()->contactID().toStdString());
    message.set_message_text(messageText.toStdString());
    QByteArray sig = signData(messageText.toUtf8() + timestamp.toString().toUtf8());
    message.set_signature(sig.constData(), sig.size());
    if (!verifyPacket(message)) {
        BUG() << "Just made a GroupMessage packet, but it didn't verify";
        return;
    }
    GroupMessageMonitor *monitor = new GroupMessageMonitor(message, m_groupMembers, this);
    connect(monitor, &GroupMessageMonitor::messageMonitorDone, this, &Group::onMessageMonitorDone);
    m_messageMonitors.append(monitor);
    m_messageHistory.insert(message);
    foreach (auto member, m_groupMembers)
    {
        connect(member, &GroupMember::groupMessageAcknowledged, monitor, &GroupMessageMonitor::onGroupMessageAcknowledged);
        member->sendMessage(message);
    }
}

void Group::beginProtocolInvite(ContactUser *contact)
{
    if (m_state != State::Good) {
        qWarning() << "Not inviting" << contact->contactID() << "because in state" << m_state;
        return;
    }
    m_state = State::PendingInvite;
    GroupMember *member = new GroupMember(contact);
    member->setState(GroupMember::State::PendingInvite);
    addGroupMember(member);
    Protocol::Data::GroupInvite::Invite invite;
    QString message = QString::fromStdString("Hi. Plz join");
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    QByteArray sig = signData(message.toUtf8() + timestamp.toString().toUtf8());
    invite.set_timestamp(timestamp.toMSecsSinceEpoch());
    invite.set_signature(sig.constData(), sig.size());
    invite.set_message_text(message.toStdString());
    invite.set_author(selfIdentity()->contactID().toStdString());
    QByteArray publicKey = selfIdentity()->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
    invite.set_public_key(publicKey.constData(), publicKey.size());
    if (!verifyPacket(invite)) {
        BUG() << "Just made a Invite packet, but it didn't verify";
        return;
    }
    GroupInviteMonitor *monitor = new GroupInviteMonitor(invite, member, this);
    connect(monitor, &GroupInviteMonitor::inviteMonitorDone, this, &Group::onInviteMonitorDone);
    connect(member, &GroupMember::groupInviteAcknowledged, monitor, &GroupInviteMonitor::onGroupInviteAcknowledged);
    member->sendInvite(invite);
}
void Group::beginProtocolIntroduction(Protocol::Data::GroupInvite::InviteResponse response)
{
    if (m_groupMembers.size() < 3) {
        onIntroductionMonitorDone(nullptr, true);
        return;
    }
    Protocol::Data::GroupInvite::InviteResponse *inviteResponse = new Protocol::Data::GroupInvite::InviteResponse();
    inviteResponse->set_accepted(response.accepted());
    inviteResponse->set_author(response.author());
    inviteResponse->set_message_text(response.message_text());
    inviteResponse->set_timestamp(response.timestamp());
    inviteResponse->set_signature(response.signature());
    Protocol::Data::GroupMeta::Introduction introduction;
    introduction.set_allocated_invite_response(inviteResponse);
    introduction.set_author(selfIdentity()->contactID().toStdString());
    introduction.set_message_text("Invite this guy");
    introduction.set_message_id(SecureRNG::randomInt(UINT_MAX));
    m_state = State::PendingIntroduction;
    GroupIntroductionMonitor *monitor = new GroupIntroductionMonitor(introduction, m_groupMembers, this);
    connect(monitor, &GroupIntroductionMonitor::introductionMonitorDone, this, &Group::onIntroductionMonitorDone);
    foreach (auto member, m_groupMembers)
    {
        connect(member, &GroupMember::groupIntroductionResponseReceived, monitor, &GroupIntroductionMonitor::onIntroductionResponseReceived);
        member->sendIntroduction(introduction);
    }
}

void Group::beginProtocolForwardMessage(Protocol::Data::GroupChat::GroupMessage &message)
{

}

GroupMember *Group::groupMemberFromRicochetId(QString ricochetId)
{
    foreach (auto member, m_groupMembers)
    {
            if (member->ricochetId() == ricochetId)
                return member;
    }
    return nullptr;
}

void Group::onChannelOpen(Protocol::Channel *channel)
{
    ContactUser *contact = contactsManager->contactUserFromChannel(channel);
    if (!contact) return;
    GroupMember *member = groupMemberFromRicochetId(contact->contactID());
    if (!member) return;
    member->connectIncomingSignals();
}

void Group::onGroupMessageReceived(Protocol::Data::GroupChat::GroupMessage &message)
{
    if (!verifyPacket(message)) {
        qWarning() << "Group::onGroupMessageReceived message didn't verify and shouldn't have gotten to here";
        qDebug() << "☓" << QDateTime::fromMSecsSinceEpoch(message.timestamp()).toString() <<
                        m_groupMembers[QString::fromStdString(message.author())]->nickname() <<
                        QString::fromStdString(message.message_text());
        return;
    }
    qDebug() << "✓" << QDateTime::fromMSecsSinceEpoch(message.timestamp()).toString() <<
                    m_groupMembers[QString::fromStdString(message.author())]->nickname() <<
                    QString::fromStdString(message.message_text());
    m_messageHistory.insert(message);
    beginProtocolForwardMessage(message);
    return;
}

void Group::onInviteReceived(Protocol::Data::GroupInvite::Invite &invite)
{
    if (!verifyPacket(invite)) {
        qWarning() << "Group::onInviteReceived invite didn't verify and shouldn't have gotten to here.";
        return;
    }
    else {
        Protocol::Data::GroupInvite::InviteResponse response;
        QString message = QString::fromStdString("Ok. Sure!");
        QDateTime timestamp = QDateTime::currentDateTimeUtc();
        QByteArray sig = signData(message.toUtf8() + timestamp.toString().toUtf8());
        QByteArray publicKey = selfIdentity()->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
        response.set_signature(sig.constData(), sig.size());
        response.set_timestamp(timestamp.toMSecsSinceEpoch());
        response.set_accepted(true);
        response.set_author(selfIdentity()->contactID().toStdString());
        response.set_public_key(publicKey.constData(), publicKey.size());
        response.set_message_text(message.toStdString());
        m_groupMembers[QString::fromStdString(invite.author())]->sendInviteResponse(response);
    }

}

void Group::onMessageMonitorDone(GroupMessageMonitor *monitor, bool totalAcknowledgement)
{
    if (totalAcknowledgement)
        qDebug() << "Group::onMessageMonitorDone everyone got:" << QString::fromStdString(monitor->message().message_text());
    else
        qDebug() << "Group::onMessageMonitorDone not everyone got:" << QString::fromStdString(monitor->message().message_text());
    m_messageMonitors.removeOne(monitor);
    delete monitor;
}

void Group::onInviteMonitorDone(GroupInviteMonitor *monitor, GroupMember *member, bool responseReceived)
{
    if (responseReceived)
        qDebug() << "Group::onInviteMonitorDone:" << QString::fromStdString(monitor->inviteResponse().message_text());
    else
        qDebug() << "Group::onInviteMonitorDone: no response";
    member->setState(GroupMember::State::PendingIntroduction);
    beginProtocolIntroduction(monitor->inviteResponse());
    delete monitor;
}

void Group::onIntroductionMonitorDone(GroupIntroductionMonitor *monitor, bool totalAcceptance)
{
    qDebug() << "Group::onIntroductionMonitorDone" << (totalAcceptance ? "accepted" : "denied");
    //m_state = State::Rebalancing;
    delete monitor;
}
