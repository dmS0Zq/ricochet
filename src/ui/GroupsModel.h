#ifndef GROUPSMODEL_H
#define GROUPSMODEL_H

#include "core/GroupsManager.h"
#include <QAbstractListModel>
#include <QList>

class UserIdentity;
class Group;

class GroupsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupsModel)

    Q_PROPERTY(GroupsManager* manager READ manager WRITE setManager NOTIFY managerChanged)
public:
    explicit GroupsModel(QObject *parent = 0);

    GroupsManager *manager() const { return m_manager; }
    void setManager(GroupsManager *manager);

    Q_INVOKABLE QModelIndex indexOfGroup(Group *group) const;
    Q_INVOKABLE int rowOfGroup(Group *group) const { return indexOfGroup(group).row(); }
    Q_INVOKABLE Group *group(int row) const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QHash<int,QByteArray> roleNames() const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void managerChanged();
private slots:
    void updateGroup(Group *group = 0);
    void groupAdded(Group *group);
private:
    QList<Group*> m_groups;
    GroupsManager *m_manager;
    void connectSignals(Group *group);
};

#endif // GROUPSMODEL_H
