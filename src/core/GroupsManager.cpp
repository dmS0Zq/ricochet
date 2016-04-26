#include "GroupsManager.h"
#include "core/ContactsManager.h"
#include "core/GroupMember.h"
#include "core/IdentityManager.h"
#include "tor/HiddenService.h"

GroupsManager *groupsManager = 0;

GroupsManager::GroupsManager(QObject *parent) : QObject(parent)
{
    groupsManager = this;
    highestID = -1;
    createGroup(QString::fromStdString("Testing Group"));
}

GroupsManager::~GroupsManager()
{
    groupsManager = 0;
}

void GroupsManager::connectSignals(Group *group)
{
    connect(contactsManager, SIGNAL(contactAdded(ContactUser*)), group, SLOT(onContactAdded(ContactUser*)));
    connect(contactsManager, SIGNAL(contactStatusChanged(ContactUser*,int)), group, SLOT(onContactStatusChanged(ContactUser*,int)));
}

Group *GroupsManager::addGroup(const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    highestID++;
    Group *group = Group::addNewGroup(highestID, this);
    group->setParent(this);
    group->setName(name);
    addSelfToGroup(group);
    connectSignals(group);

    qDebug() << "Added new group " << group->name() << " with ID " << group->m_uniqueID;
    group->setState(Group::State::Good);
    groups.append(group);
    emit groupAdded(group);

    return group;
}

void GroupsManager::removeGroup(Group *group)
{
    int index = groups.indexOf(group);
    if (index != -1)
    {
        groups.removeAt(index);
        emit groupRemoved(group);
    }
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

void GroupsManager::addSelfToGroup(Group *group)
{
    GroupMember *member = new GroupMember(identityManager->identities().at(0)); // assume 1 identity
    group->addGroupMember(member);
}

Group *GroupsManager::groupFromChannel(const Protocol::Channel *channel)
{
    for (auto group = groups.begin(); group != groups.end(); group++) {
        auto members = (*group)->groupMembers();
        for (auto member = members.begin(); member != members.end(); member++) {
            auto channels = (*member)->channels();
            for (auto chan = channels.begin(); chan != channels.end(); chan++) {
                if ((*chan) == channel) {
                    return (*group);
                }
            }
        }
    }
    return nullptr;
}
