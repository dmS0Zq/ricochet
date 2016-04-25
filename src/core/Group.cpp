#include "Group.h"
#include "core/IdentityManager.h"
#include "protocol/ControlChannel.h"
#include "protocol/GroupChatChannel.h"
#include "protocol/GroupInviteChannel.h"
#include "tor/HiddenService.h"
#include <QCryptographicHash>
#include <QDebug>

Group::Group(int id, QObject *parent)
    : QObject(parent)
    , m_uniqueID(id)
    , m_state(State::Undefined)
{
    Q_ASSERT(m_uniqueID >= 0);
}

void Group::onContactAdded(ContactUser *user)
{
    qDebug() << name() << " knows about new contact";
}

void Group::testSendMessage()
{
    Protocol::Data::GroupChat::GroupMessage chatMessage;
    QString author = selfIdentity()->contactID();
    QString text = QString::fromStdString("Ayy LMAO");
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    chatMessage.set_author(author.toStdString());
    chatMessage.set_message_text(text.toStdString());
    chatMessage.set_timestamp(timestamp.toMSecsSinceEpoch());
    QByteArray sig = signData(text.toUtf8() + timestamp.toString().toUtf8());
    chatMessage.set_signature(sig.constData(), sig.size());
    sendMessage(chatMessage);
}

void Group::sendMessage(Protocol::Data::GroupChat::GroupMessage message)
{
    if (!verifyPacket(message)) {
        qWarning() << "Refusing to send a packet that doesn't verify";
        return;
    }
    GroupMessageMonitor *gmm = new GroupMessageMonitor(message, m_groupMembers, this);
    m_messageMonitors.append(gmm);
    foreach (auto member, m_groupMembers)
    {
        connect(member, &GroupMember::groupMessageAcknowledged, gmm, &GroupMessageMonitor::onGroupMessageAcknowledged);
        member->sendMessage(message);
    }
    connect(gmm, &GroupMessageMonitor::messageMonitorDone, this, &Group::onMessageMonitorDone);
    m_messageHistory.insert(message);
}

void Group::onContactStatusChanged(ContactUser *user, int status)
{
    qDebug() << name() << " knows about status change";
    if (status == ContactUser::Status::Online)
    {
        if (!m_groupMembers.contains(user->contactID()))
        {
            GroupMember *member = new GroupMember(user);
            addGroupMember(member);
            Protocol::Data::GroupInvite::Invite *invite = new Protocol::Data::GroupInvite::Invite();
            QString message = QString::fromStdString("Hi. Plz join");
            QDateTime timestamp = QDateTime::currentDateTimeUtc();
            invite->set_author(selfIdentity()->contactID().toStdString());
            invite->set_message_text(message.toStdString());
            invite->set_timestamp(timestamp.toMSecsSinceEpoch());
            QByteArray sig = signData(message.toUtf8() + timestamp.toString().toUtf8());
            invite->set_signature(sig.constData(), sig.size());
            QByteArray publicKey = selfIdentity()->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
            invite->set_public_key(publicKey.constData(), publicKey.size());
            member->sendInvite(invite);
        }
    }
    else if (status == ContactUser::Status::Offline)
    {
        if (m_groupMembers.contains(user->contactID()))
        {
            GroupMember *member = m_groupMembers[user->contactID()];
            //m_groupMembers.remove(member->ricochetId());
            if (m_groupMembers.size() < 2)
            {
                //groupsManager->removeGroup(this);
            }
        }
    }
    else
    {
        qDebug() << user->nickname() << " went away or something is wrong";
    }
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
        qDebug() << "Adding" << member->ricochetId() << "to group";
        m_groupMembers[member->ricochetId()] = member;
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

void Group::begingProtocolIntroduction(const Protocol::Data::GroupInvite::InviteResponse &inviteResponse)
{
    setState(State::Introduction);

}

void Group::onMessageMonitorDone(GroupMessageMonitor *monitor, bool totalAcknowledgement)
{
    if (totalAcknowledgement)
        qDebug() << "Everyone got:" << QString::fromStdString(monitor->message().message_text());
    else
        qDebug() << "Not everyone got:" << QString::fromStdString(monitor->message().message_text());
    m_messageMonitors.removeOne(monitor);
    delete monitor;
}
