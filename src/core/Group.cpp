#include "Group.h"
#include <QCryptographicHash>
#include <QDebug>

Group::Group(int id, QObject *parent)
    : QObject(parent)
    , m_uniqueID(id)
    , m_settings(0)
{
    Q_ASSERT(m_uniqueID >= 0);
    m_settings = new SettingsObject(QStringLiteral("groups.%1").arg(m_uniqueID));
    connect(m_settings, &SettingsObject::modified, this, &Group::onSettingsModified);
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
