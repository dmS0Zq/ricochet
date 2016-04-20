#include "GroupsManager.h"
#include "core/ContactsManager.h"
#include "core/IdentityManager.h"
#include "tor/HiddenService.h"

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
    connect(contactsManager, SIGNAL(contactAdded(ContactUser*)), group, SLOT(onContactAdded(ContactUser*)));
    connect(contactsManager, SIGNAL(contactStatusChanged(ContactUser*,int)), group, SLOT(onContactStatusChanged(ContactUser*,int)));
}

Group *GroupsManager::addGroup(const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    highestID++;
    Group *group = Group::addNewGroup(highestID);
    group->setParent(this);
    group->setName(name);
    addSelfToGroup(group);
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

void GroupsManager::addSelfToGroup(Group *group)
{
    Group::GroupMember *member = new Group::GroupMember(identityManager->identities().at(0)); // assume 1 identity
    UserIdentity *self = identityManager->identities().at(0); // assume 1 identity
    QByteArray privateKey = self->hiddenService()->privateKey().encodedPrivateKey(CryptoKey::PEM);
    group->addGroupMember(member);
    group->setSelfIdentity(self);
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
