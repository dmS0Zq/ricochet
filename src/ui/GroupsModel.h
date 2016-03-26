#ifndef GROUPSMODEL_H
#define GROUPSMODEL_H

#include "core/GroupManager.h"
#include <QAbstractListModel>
#include <QList>

class UserIdentity;
class Group;

class GroupsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupsModel)

    Q_PROPERTY(GroupManager* manager READ manager WRITE setManager NOTIFY managerChanged)
public:
    explicit GroupsModel(QObject *parent = 0);

    Q_INVOKABLE QModelIndex indexOfGroup(Group *group) const;
    Q_INVOKABLE int rowOfGroup(Group *group) const { return indexOfGroup(group).row(); }

    GroupManager *manager() const { return m_manager; }
    void setManager(GroupManager *manager);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

signals:
    void managerChanged();

private:
    QList<Group*> groups;
    GroupManager *m_manager;
};

#endif // GROUPSMODEL_H
