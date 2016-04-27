#include "Group.h"
#include "core/ContactIDValidator.h"
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
        connect(member->connection(), &Protocol::Connection::channelOpened, this, &Group::onChannelOpen);
        connect(member, &GroupMember::groupMessageReceived, this, &Group::onGroupMessageReceived);
        connect(member, &GroupMember::groupIntroductionReceived, this, &Group::onGroupIntroductionReceived);
    }
    else {
        qDebug() << member->ricochetId() << "already in group";
    }
    qDebug() << "Group members: ";
    foreach (auto member, m_groupMembers) {
        qDebug() << "    " << member->nickname();
    }
}

QByteArray Group::signData(const QByteArray &data)
{
    return selfIdentity()->hiddenService()->privateKey().signData(data);
}

Group *Group::addNewGroup(int id, QObject *parent)
{
    Group *group = new Group(id, parent);
    group->setName(QString());
    return group;
}


bool Group::verifyPacket(const Protocol::Data::GroupInvite::Invite &packet)
{
    if (!packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_message_text() ||
            !packet.has_author() ||
            !packet.has_public_key()) {
        return false;
    }
    // verify public key data is valid
    CryptoKey key;
    QByteArray keyData = QByteArray::fromStdString(packet.public_key());
    if (!key.loadFromData(keyData, CryptoKey::PublicKey, CryptoKey::DER)) {
        return false;
    }
    // verify public key resolves to claimed author
    QString author = QString::fromStdString(packet.author());
    if (QString::fromStdString("ricochet:") + key.torServiceID() != author) {
        return false;
    }
    // verify time isn't too long ago
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC();
    if (timestamp < QDateTime::fromMSecsSinceEpoch(1)) {
        return false;
    }
    // verify signature is correct
    QByteArray signature = QByteArray::fromStdString(packet.signature());
    QString messageText = QString::fromStdString(packet.message_text());
    if (!key.verifyData(messageText.toUtf8() + timestamp.toString().toUtf8(), signature)) {
        return false;
    }
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupInvite::InviteResponse &packet) {
    if (!packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_accepted() ||
            !packet.has_author() ||
            !packet.has_public_key() ||
            !packet.has_message_text()) {
        qDebug() << "Group::verifyPacket InviteResponse missing a piece";
        return false;
    }
    // verify public key data is valid
    CryptoKey key;
    QByteArray keyData = QByteArray::fromStdString(packet.public_key());
    if (!key.loadFromData(keyData, CryptoKey::PublicKey, CryptoKey::DER)) {
        qDebug() << "Group::verifyPacket InviteResponse bad key data";
        return false;
    }
    // verify public key resolves to claimed author
    QString author = QString::fromStdString(packet.author());
    if (QString::fromStdString("ricochet:") + key.torServiceID() != author) {
        qDebug() << "Group::verifyPacket InviteResponse public key not to right author";
        return false;
    }
    // verify time isn't too long ago
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC();
    if (timestamp < QDateTime::fromMSecsSinceEpoch(1)) {
        qDebug() << "Group::verifyPacket InviteResponse timestamp bad";
        return false;
    }
    QByteArray signature = QByteArray::fromStdString(packet.signature());
    QString messageText = QString::fromStdString(packet.message_text());
    if (!key.verifyData(messageText.toUtf8() + timestamp.toString().toUtf8(), signature)) {
        qDebug() << "Group::verifyPacket InviteResponse signature bad";
        return false;
    }
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupInvite::IntroductionAccepted &packet)
{
    if (!packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_author() ||
            !packet.has_invite_response() ||
            !verifyPacket(packet.invite_response()) ||
            packet.member_size() != packet.member_public_key_size() ||
            packet.member_size() != packet.member_signature_size()) {
        return false;
    }
    QByteArray data = QByteArray::fromStdString(packet.author());
    data += QByteArray::fromStdString(QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString().toStdString());
    for (int i = 0; i < packet.member_size(); i++) {
        // verify member_signature
        bool accepted = true;
        CryptoKey k;
        QByteArray s = QByteArray::fromStdString(packet.member_signature(i));
        QByteArray d = QByteArray::fromStdString(packet.member(i));
        d += QByteArray::fromRawData(packet.invite_response().SerializeAsString().data(), packet.invite_response().SerializeAsString().size());
        d += QByteArray::fromRawData((char*)&accepted, sizeof(bool));
        if (QString::fromStdString(packet.member(i)) == selfIdentity()->contactID()) {
            k = selfIdentity()->hiddenService()->privateKey();
        } else {
            k = contactsManager->lookupHostname(QString::fromStdString(packet.member(i)))->publicKey();
            if (k.publicKeyDigest() == CryptoKey().publicKeyDigest()) {
                QByteArray keyData = QByteArray::fromStdString(packet.member_public_key(i));
                if (!k.loadFromData(keyData, CryptoKey::PublicKey, CryptoKey::DER)) {
                    return false;
                } else if (QString::fromStdString("ricochet:")+k.torServiceID() != QString::fromStdString(packet.member(i))) {
                    return false;
                } else {
                    contactsManager->lookupHostname(QString::fromStdString(packet.member(i)))->setPublicKey(k);
                }
            }
        }
        if (!k.verifyData(d, s)) {
            return false;
        }
        // member signature good, so add it to data
        data += QByteArray::fromStdString(packet.member(i));
        data += s;
    }
    QByteArray signature = QByteArray::fromStdString(packet.signature());
    CryptoKey key;
    if (selfIdentity()->contactID() == QString::fromStdString(packet.author())) {
        key = selfIdentity()->hiddenService()->privateKey();
    } else {
        QString hostname = ContactIDValidator::hostnameFromID(QString::fromStdString(packet.author()));
        key = contactsManager->lookupHostname(hostname)->publicKey();
    }
    if (!key.verifyData(data, signature)) {
        return false;
    }
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupMeta::Introduction &packet)
{
    if (!packet.has_invite_response() ||
            !verifyPacket(packet.invite_response()) ||
            !packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_author() ||
            !packet.has_message_text()) {
        return false;
    }
    QByteArray data = QByteArray::fromStdString(packet.message_text());
    data += QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString().toUtf8();
    data += QByteArray::fromRawData(packet.invite_response().SerializeAsString().data(), packet.invite_response().SerializeAsString().size());
    QByteArray signature = QByteArray::fromStdString(packet.signature());
    QString author = QString::fromStdString(packet.author());
    CryptoKey key;
    if (selfIdentity()->contactID() == QString::fromStdString(packet.author())) {
        key = selfIdentity()->hiddenService()->privateKey();
    } else {
        QString hostname = ContactIDValidator::hostnameFromID(author);
        key = contactsManager->lookupHostname(hostname)->publicKey();
    }
    if (!key.verifyData(data, signature)) {
        return false;
    }
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupMeta::IntroductionResponse &packet)
{
    if (!packet.has_invite_response() ||
            !verifyPacket(packet.invite_response()) ||
            !packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_accepted() ||
            !packet.has_author()) {
        return false;
    }
    QByteArray signature = QByteArray::fromStdString(packet.signature());
    bool accepted = packet.accepted();
    QString author = QString::fromStdString(packet.author());
    QByteArray data = author.toUtf8();
    data += QByteArray::fromRawData(packet.invite_response().SerializeAsString().data(), packet.invite_response().SerializeAsString().size());
    data += QByteArray::fromRawData((char*)&accepted, sizeof(bool));
    qDebug() << "data length:" << data.size();
    CryptoKey key;
    if (selfIdentity()->contactID() == author) {
        key = selfIdentity()->hiddenService()->privateKey();
    } else {
        QString hostname = ContactIDValidator::hostnameFromID(author);
        key = contactsManager->lookupHostname(hostname)->publicKey();
    }
    if (!key.verifyData(data, signature)) {
        return false;
    }
    return true;
}

bool Group::verifyPacket(const Protocol::Data::GroupChat::GroupMessage &packet)
{
    if (!packet.has_message_id() ||
            !packet.has_signature() ||
            !packet.has_timestamp() ||
            !packet.has_author() ||
            !packet.has_message_text()) {
        return false;
    }
    BUG() << "Group::verifyPacket GroupMessage not checked";
    //QString text = QString::fromStdString(packet.message_text());
    //QString time = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString();
    //QByteArray data = QByteArray::fromStdString(text.toStdString() + time.toStdString());
    //QString author = QString::fromStdString(packet.author());
    //QByteArray sig = QByteArray::fromStdString(packet.signature());
    //return m_groupMembers[author]->key().verifyData(data, sig);
    return false;
}

bool verifyPacket(const Protocol::Data::GroupChat::GroupMessageAcknowledge &packet)
{
    if (!packet.has_message_id() ||
            !packet.has_accepted()) {
        return false;
    }
    BUG() << "Group::verifyPacket GroupMessageAcknowledge not implemeneted";
    return true;
}

UserIdentity *Group::selfIdentity()
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
    using Protocol::Data::GroupInvite::Invite;
    if (m_state != State::Good) {
        qWarning() << "Not inviting" << contact->contactID() << "because in state" << m_state;
        return;
    }
    GroupMember *inviter = new GroupMember(selfIdentity());
    GroupMember *invitee = new GroupMember(contact);
    Invite invite;
    QByteArray signature;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    QString messageText = QString::fromStdString("Hi. Please joing a group chat.");
    QString author = inviter->ricochetId();
    QByteArray publicKey = inviter->key().encodedPublicKey(CryptoKey::DER);
    signature = signData(messageText.toUtf8() + timestamp.toString().toUtf8());
    invite.set_signature(signature.constData(), signature.size());
    invite.set_timestamp(timestamp.toMSecsSinceEpoch());
    invite.set_message_text(messageText.toStdString());
    invite.set_author(author.toStdString());
    invite.set_public_key(publicKey.constData(), publicKey.size());
    GroupInviteMonitor *monitor = new GroupInviteMonitor(invite, invitee, inviter, this, this);
    m_state = State::PendingInvite;
    if (!monitor->go()) {
        qWarning() << "Group::beginProtocolInvite could not send invite";
    }
}
void Group::beginProtocolIntroduction(const Protocol::Data::GroupInvite::InviteResponse &response, GroupMember *invitee)
{
    //if (m_groupMembers.size() < 2) {
    //    qDebug() << "Group::beginProtocolIntroduction small group, so already done";
    //    onIntroductionMonitorDone(nullptr, invitee, true);
    //    return;
    //}
    Protocol::Data::GroupInvite::InviteResponse *inviteResponse = new Protocol::Data::GroupInvite::InviteResponse();
    inviteResponse->set_signature(response.signature());
    inviteResponse->set_timestamp(response.timestamp());
    inviteResponse->set_accepted(response.accepted());
    inviteResponse->set_author(response.author());
    inviteResponse->set_public_key(response.public_key());
    inviteResponse->set_message_text(response.message_text());
    Protocol::Data::GroupMeta::Introduction introduction;
    introduction.set_allocated_invite_response(inviteResponse);
    QByteArray signature;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    QString author = selfIdentity()->contactID();
    QString messageText = QString::fromStdString("Allow this guy in.");
    QByteArray data = messageText.toUtf8();
    data += timestamp.toString().toUtf8();
    data += QByteArray::fromRawData(inviteResponse->SerializeAsString().data(), inviteResponse->SerializeAsString().size());
    signature = signData(data);
    introduction.set_signature(signature.constData(), signature.size());
    introduction.set_timestamp(timestamp.toMSecsSinceEpoch());
    introduction.set_author(author.toStdString());
    introduction.set_message_text(messageText.toStdString());
    if (!verifyPacket(introduction)) {
        BUG() << "Just made an Introduction packet but it didn't verify";
        return;
    }
    m_state = State::PendingIntroduction;
    GroupIntroductionMonitor *monitor = new GroupIntroductionMonitor(introduction, invitee, this, m_groupMembers, this);
    if (!monitor->go()) {
        qWarning() << "Group::beginProtocolIntroduction could not send introduction";
    }
}

void Group::beginProtocolForwardMessage(Protocol::Data::GroupChat::GroupMessage &message)
{
    Q_UNUSED(message)
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

void Group::onGroupIntroductionReceived(Protocol::Data::GroupMeta::Introduction &introduction)
{
    if (!verifyPacket(introduction)) {
        qWarning() << "Group::onGroupIntroductionReceived introduction didn't verify and shouldn't have gotten to here.";
        return;
    }
    QString hostname = QString::fromStdString(introduction.invite_response().author());
    ContactUser *invitedUser = contactsManager->lookupHostname(hostname);
    if (!invitedUser)
        return;
    GroupMember *member = new GroupMember(invitedUser);
    addGroupMember(member);
    Protocol::Data::GroupMeta::IntroductionResponse response;
    Protocol::Data::GroupInvite::InviteResponse *inviteResponse = new Protocol::Data::GroupInvite::InviteResponse();
    inviteResponse->set_signature(introduction.invite_response().signature());
    inviteResponse->set_timestamp(introduction.invite_response().timestamp());
    inviteResponse->set_accepted(introduction.invite_response().accepted());
    inviteResponse->set_author(introduction.invite_response().author());
    inviteResponse->set_public_key(introduction.invite_response().public_key());
    inviteResponse->set_message_text(introduction.invite_response().message_text());
    QByteArray signature;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    bool accepted = true;
    QString author = selfIdentity()->contactID();
    QByteArray data = author.toUtf8();
    data += QByteArray::fromRawData(inviteResponse->SerializeAsString().data(), inviteResponse->SerializeAsString().size());
    data += QByteArray::fromRawData((char*)&accepted, sizeof(bool));
    signature = signData(data);
    response.set_allocated_invite_response(inviteResponse);
    response.set_signature(signature.constData(), signature.size());
    response.set_timestamp(timestamp.toMSecsSinceEpoch());
    response.set_accepted(accepted);
    response.set_author(author.toStdString());
    if (!verifyPacket(response)) {
        BUG() << "Group::onGroupIntroductionReceived just make introduction response and it didn't verify";
        return;
    }
    foreach (auto member, m_groupMembers)
    {
        if (member->ricochetId() != selfIdentity()->contactID())
            member->sendIntroductionResponse(response);
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

void Group::onInviteMonitorDone(GroupInviteMonitor *monitor, bool responseReceived)
{
    if (!responseReceived) {
        qDebug() << "Group::onInviteMonitorDone: no response";
        return;
    }
    qDebug() << "Group::onInviteMonitorDone:" << QString::fromStdString(monitor->response().message_text());
    if (!monitor->invitee()->setKey(QByteArray::fromStdString(monitor->response().public_key()))) {
        qWarning() << "Group::onInviteMonitorDone ignoring invite response with a public key we don't accept";
        return;
    }
    beginProtocolIntroduction(monitor->response(), monitor->invitee());
    delete monitor;
}

void Group::onIntroductionMonitorDone(GroupIntroductionMonitor *monitor, GroupMember *invitee, bool totalAcceptance)
{
    qDebug() << "Group::onIntroductionMonitorDone" << (totalAcceptance ? "accepted" : "denied");
    Protocol::Data::GroupInvite::IntroductionAccepted accepted;
    QByteArray signature;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    QString author = selfIdentity()->contactID();
    QByteArray data = author.toUtf8() + timestamp.toString().toUtf8();
    // for each member, add their stuff to the data
    foreach (auto member, m_groupMembers) {
        if (member->ricochetId() != selfIdentity()->contactID()) {
            data += member->ricochetId().toUtf8();
            data += monitor->introductionResponseSignatures()[member->ricochetId()];
        }
    }
    //
    signature = signData(data);
    accepted.set_signature(signature.constData(), signature.size());
    accepted.set_timestamp(timestamp.toMSecsSinceEpoch());
    accepted.set_author(author.toStdString());
    Protocol::Data::GroupInvite::InviteResponse *inviteResponse = new Protocol::Data::GroupInvite::InviteResponse();
    inviteResponse->set_signature(monitor->inviteResponse().signature());
    inviteResponse->set_timestamp(monitor->inviteResponse().timestamp());
    inviteResponse->set_accepted(monitor->inviteResponse().accepted());
    inviteResponse->set_author(monitor->inviteResponse().author());
    inviteResponse->set_public_key(monitor->inviteResponse().public_key());
    inviteResponse->set_message_text(monitor->inviteResponse().message_text());
    accepted.set_allocated_invite_response(inviteResponse);
    // for each member, add their stuff to accepted
    foreach (auto member, m_groupMembers) {
        if (member->ricochetId() != selfIdentity()->contactID()) {
            accepted.add_member(member->ricochetId().toStdString());
            QByteArray k = member->key().encodedPublicKey(CryptoKey::DER);
            accepted.add_member_public_key(k.constData(), k.size());
            QByteArray s = monitor->introductionResponseSignatures()[member->ricochetId()];
            accepted.add_member_signature(s.constData(), s.size());
        }
    }
    //
    if (!verifyPacket(accepted)) {
        BUG() << "Group::onIntroductionMonitorDone just made a IntroductionAccepted and it didn't verify";
        return;
    }
    addGroupMember(invitee);
    invitee->sendIntroductionAccepted(accepted);
    //m_state = State::Rebalancing;
    m_state = State::Good;
    delete monitor;
}
