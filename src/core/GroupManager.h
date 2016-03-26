#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include "Group.h"
#include <QObject>
#include <QDebug>

class GroupManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupManager)

public:
    Q_INVOKABLE void test() { qDebug() << "TEST TEST TEST"; }
    explicit GroupManager(QObject *parent = 0);
    ~GroupManager();
private:
    QList<Group> m_groups;
};

Q_DECLARE_METATYPE(GroupManager*)

extern GroupManager *groupManager;
#endif // GROUPMANAGER_H
