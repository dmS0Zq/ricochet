#include "GroupsManager.h"
#include "core/ContactsManager.h"
#include "core/GroupMember.h"
#include "core/IdentityManager.h"
#include "tor/HiddenService.h"
#include "utils/Useful.h"

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

void GroupsManager::onInviteReceived(const Protocol::Data::GroupInvite::Invite &invite)
{
    // assume user wants to join group
    GroupMember *member = new GroupMember(contactsManager->lookupHostname(QString::fromStdString(invite.author())));
    if (!member->setKey(QByteArray::fromStdString(invite.public_key()))) {
        qWarning() << "GroupsManager:onInviteReceived could not set member key";
        return;
    }
    Protocol::Data::GroupInvite::InviteResponse response;
    QByteArray signature;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    bool accepted = true;
    QString author = Group::selfIdentity()->contactID();
    QByteArray publicKey = Group::selfIdentity()->hiddenService()->privateKey().encodedPublicKey(CryptoKey::DER);
    QString messageText = QString::fromStdString("Sure. I'll join, if everyone will have me.");
    signature = Group::signData(messageText.toUtf8() + timestamp.toString().toUtf8());
    response.set_signature(signature.constData(), signature.size());
    response.set_timestamp(timestamp.toMSecsSinceEpoch());
    response.set_accepted(accepted);
    response.set_author(author.toStdString());
    response.set_public_key(publicKey.constData(), publicKey.size());
    response.set_message_text(messageText.toStdString());
    if (!Group::verifyPacket(response)) {
        BUG() << "GroupsMonitor::onInviteReceived just created response and signature didnt' verify";
        return;
    }
    member->sendInviteResponse(response);
}

void GroupsManager::onIntroductionAcceptedReceived(const Protocol::Data::GroupInvite::IntroductionAccepted &accepted)
{
    if (!Group::verifyPacket(accepted)) {
        BUG() << "GroupsManager::onIntroductionAcceptedReceived got packet that didn't verify and shouldn't have gotten here.";
        return;
    }
    Group *group = test_getTestingGroup();
    GroupMember *member = new GroupMember(contactsManager->lookupHostname(QString::fromStdString(accepted.author())));
    group->addGroupMember(member);
    for (int i = 0; i < accepted.member_size(); i++) {
        member = new GroupMember(contactsManager->lookupHostname(QString::fromStdString(accepted.member(i))));
        group->addGroupMember(member);
    }
}
