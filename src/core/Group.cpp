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
    , m_state(State::Unknown)
{
    Q_ASSERT(m_uniqueID >= 0);
}

void Group::onContactAdded(ContactUser *user)
{
    qDebug() << name() << " knows about new contact";
}

void Group::onContactStatusChanged(ContactUser *user, int status)
{
    qDebug() << name() << " knows about status change";
    if (status == ContactUser::Status::Online)
    {
        if (!m_groupMembers.contains(user->contactID()))
        {
            GroupMember *member = new GroupMember();
            member->ricochetId = user->contactID();
            member->key = user->publicKey();
            member->connection = user->connection();
            addGroupMember(member);
            QString message = QString::fromStdString("hi");
            QDateTime time = QDateTime::currentDateTimeUtc();
            QByteArray signature = signData(message.toUtf8() + time.toString().toUtf8());
            QByteArray publicKey = m_selfIdentity->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
            Protocol::GroupInviteChannel *channel = new Protocol::GroupInviteChannel(Protocol::Channel::Outbound, member->connection.data());
            channel->openChannel();
            channel->sendInvite(message, time, signature, m_selfIdentity->contactID(), publicKey);
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

QString Group::name() const
{
    return m_name;
}

QList<QByteArray> Group::seedHashes() const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QList<QByteArray> seedHashes;
    foreach (QByteArray seed, m_seeds)
    {
        hash.addData(seed);
        seedHashes.append(hash.result());
        hash.reset();
    }
    return seedHashes;
}

void Group::setName(const QString &name)
{
    m_name = name;
}

void Group::setSelfIdentity(UserIdentity *identity)
{
    m_selfIdentity = identity;
}

void Group::addGroupMember(GroupMember *member)
{
    if (!m_groupMembers.contains(member->ricochetId))
    {
        qDebug() << "Adding" << member->ricochetId << "to group";
        m_groupMembers[member->ricochetId] = member;
    }
    else {
        qDebug() << member->ricochetId << "already in group";
    }
}

QByteArray Group::signData(const QByteArray &data)
{
    CryptoKey *key = &(m_groupMembers[m_selfIdentity->contactID()]->key);
    return key->signData(data);
}

Group *Group::addNewGroup(int id)
{
    Group *group = new Group(id);
    group->setSelfIdentity(identityManager->identities().at(0)); // Assumes only one identity
    group->setName(QString());
    return group;
}

bool Group::verifyPacket(const Protocol::Data::GroupInvite::Invite &packet)
{
    if (!packet.has_message_text() || !packet.has_signature() || !packet.has_timestamp() || !packet.has_author()) {
        return false;
    }
    QString text = QString::fromStdString(packet.message_text());
    QString time = QDateTime::fromMSecsSinceEpoch(packet.timestamp()).toUTC().toString();
    QByteArray data = QByteArray::fromStdString(text.toStdString() + time.toStdString());
    QString author = QString::fromStdString(packet.author());
    QByteArray sig = QByteArray::fromStdString(packet.signature());
    return m_groupMembers[author]->key.verifyData(data, sig);
}
