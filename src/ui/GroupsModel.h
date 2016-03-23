#ifndef GROUPSMODEL_H
#define GROUPSMODEL_H

#include <QAbstractListModel>
#include <QList>

class UserIdentity;
class Group;

class GroupsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(GroupsModel)
public:
    explicit GroupsModel(QObject *parent = 0);

    Q_INVOKABLE QModelIndex indexOfGroup(Group *group) const;
    Q_INVOKABLE int rowOfGroup(Group *group) const { return indexOfGroup(group).row(); }

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

private:
    QList<Group*> groups;
};

#endif // GROUPSMODEL_H
