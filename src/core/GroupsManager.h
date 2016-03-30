#ifndef GROUPSMANAGER_H
#define GROUPSMANAGER_H

#include "Group.h"
#include <QObject>
#include <QDebug>

class GroupsManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupsManager)

public:
    Q_INVOKABLE void test() { qDebug() << "TEST TEST TEST"; }
    QList<Group*> groups;
    explicit GroupsManager(QObject *parent = 0);
    ~GroupsManager();

    /* Use this instead of addGroup */
    Q_INVOKABLE Group *createGroup(const QString &groupName);
    Group *addGroup(const QString &name);
signals:
    void groupAdded(Group *group);
private:
    int highestID;
    void connectSignals(Group *group);
};

Q_DECLARE_METATYPE(GroupsManager*)

extern GroupsManager *groupsManager;
#endif // GROUPSMANAGER_H
