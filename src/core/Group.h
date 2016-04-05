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
    Q_PROPERTY(int uniqueID READ uniqueID CONSTANT)

    friend class GroupsManager;
public:
    enum State
    {
        Ready,
        InvitationAcceptance,
        Rebalance,
        Unknown
    };
    Group(int id, QObject *parent = 0);
    QList<QByteArray> seeds() const { return m_seeds; }
    QList<QByteArray> seedHashes() const;
    QString name() const;
    SettingsObject *settings() { return m_settings; }
    void setName(const QString &name);
    int uniqueID() const { return m_uniqueID; }
signals:
    void nameChanged();
    void stateChanged(State oldState, State newState);
private slots:
    void onContactAdded(ContactUser *user);
    void onContactStatusChanged(ContactUser *user, int status);
    void onSettingsModified(const QString &key, const QJsonValue &value);
private:

    struct GroupMember
    {
        ContactUser *contactUser;
        QList<QByteArray> seeds;
        QList<QByteArray> seedHashes;
        QByteArray position;
    };
    QMap<QString, GroupMember> m_groupMembers;
    QList<QByteArray> m_seeds;
    int m_uniqueID;
    SettingsObject *m_settings;
    Group::State m_state;

    /* See GroupsManager::addGroup */
    static Group *addNewGroup(int id);
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
