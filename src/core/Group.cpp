#include "Group.h"
#include "protocol/ControlChannel.h"
#include "protocol/GroupChatChannel.h"
#include <QCryptographicHash>
#include <QDebug>

Group::Group(int id, QObject *parent)
    : QObject(parent)
    , m_uniqueID(id)
    , m_settings(0)
    , m_state(State::Unknown)
{
    Q_ASSERT(m_uniqueID >= 0);
    m_settings = new SettingsObject(QStringLiteral("groups.%1").arg(m_uniqueID));
    connect(m_settings, &SettingsObject::modified, this, &Group::onSettingsModified);
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
            Protocol::GroupChatChannel *channel = new Protocol::GroupChatChannel(Protocol::Channel::Outbound, user->connection().data());
            user->connection()->findChannel<Protocol::ControlChannel>()->sendOpenChannel(channel);
            user->connection()->findChannel<Protocol::GroupChatChannel>()->sendGroupMessage(QString::fromStdString("Hi"), QDateTime());
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
    return m_settings->read("name").toString();
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
    m_settings->write("name", name);
}

Group *Group::addNewGroup(int id)
{
    Group *group = new Group(id);
    group->settings()->write("whenCreated", QDateTime::currentDateTime());

    return group;
}
