#include "GroupsModel.h"

GroupsModel::GroupsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_manager = groupManager;
}

QModelIndex GroupsModel::indexOfGroup(Group *group) const
{
    int row = groups.indexOf(group);
    if (row < 0)
        return QModelIndex();
    return index(row, 0);
}

int GroupsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return groups.size();
}

QVariant GroupsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= groups.size())
        return QVariant();

    Group *group = groups[index.row()];
    return QVariant();
}

void GroupsModel::setManager(GroupManager *manager)
{
    if (manager == m_manager)
        return;
    m_manager = manager;
}
