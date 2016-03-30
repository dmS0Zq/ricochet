#ifndef GROUP_H
#define GROUP_H

#include "ContactUser.h"
#include <QObject>
#include <QMap>

class Group : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Group)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    friend class GroupsManager;
public:
    Group(int id, QObject *parent = 0);
    QList<QByteArray> seeds() const { return m_seeds; }
    QList<QByteArray> seedHashes() const;
    QString name() const;
    SettingsObject *settings() { return m_settings; }
    void setName(const QString &name);
signals:
    void nameChanged();
private slots:
    void onSettingsModified(const QString &key, const QJsonValue &value);
private:
    struct GroupMember
    {
        ContactUser *contactUser;
        QList<QByteArray> seeds;
        QList<QByteArray> seedHashes;
    };
    QMap<QByteArray, GroupMember> m_groupMembers;
    QList<QByteArray> m_seeds;
    int m_uniqueID;
    SettingsObject *m_settings;

    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id);
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
