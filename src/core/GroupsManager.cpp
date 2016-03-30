#include "GroupsManager.h"

GroupsManager *groupsManager = 0;

GroupsManager::GroupsManager(QObject *parent) : QObject(parent)
{
    groupsManager = this;
    highestID = -1;
}

GroupsManager::~GroupsManager()
{
    groupsManager = 0;
}

void GroupsManager::connectSignals(Group *group)
{
    //connect(group, SIGNAL(groupDeleted(Group*)), SLOT(groupDeleted(Group*)));
}

Group *GroupsManager::addGroup(const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    highestID++;
    Group *group = Group::addNewGroup(highestID);
    group->setParent(this);
    group->setName(name);
    connectSignals(group);

    qDebug() << "Added new group " << group->name() << " with ID " << group->m_uniqueID;
    groups.append(group);
    emit groupAdded(group);

    return group;
}

Group *GroupsManager::createGroup(const QString &groupName)
{
    bool b = blockSignals(true);
    Group *group = addGroup(groupName);
    blockSignals(b);
    if (!group)
        return group;
    group->setName(groupName);
    emit groupAdded(group);
    qDebug() << "Created new group" << group->name();
    return group;
}
