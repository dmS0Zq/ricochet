#include "GroupsModel.h"

inline bool groupSort(const Group *g1, const Group *g2)
{
    return g1->name().localeAwareCompare(g2->name()) < 0;
}

GroupsModel::GroupsModel(QObject *parent)
    : QAbstractListModel(parent), m_manager(0)
{

}

QModelIndex GroupsModel::indexOfGroup(Group *group) const
{
    int row = m_groups.indexOf(group);
    if (row < 0)
        return QModelIndex();
    return index(row, 0);
}

Group *GroupsModel::group(int row) const
{
    return m_groups.value(row);
}

void GroupsModel::updateGroup(Group *group)
{
    if (!group)
    {
        group = qobject_cast<Group*>(sender());
        if (!group)
            return;
    }
    int row = m_groups.indexOf(group);
    if (row < 0)
    {
        group->disconnect(this);
        return;
    }

    QList<Group*> sorted = m_groups;
    std::sort(sorted.begin(), sorted.end(), groupSort);
    int newRow = sorted.indexOf(group);

    if (row != newRow)
    {
        beginMoveRows(QModelIndex(), row, row, QModelIndex(), (newRow>row) ? (newRow+1) : newRow);
        m_groups = sorted;
        endMoveRows();
    }
    emit dataChanged(index(newRow, 0), index(newRow, 0));
}

QHash<int,QByteArray> GroupsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "name";
    roles[Qt::UserRole] = "group";
    return roles;
}

int GroupsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_groups.size();
}

QVariant GroupsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_groups.size())
        return QVariant();

    Group *group = m_groups[index.row()];
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return group->name();
    default:
        return group->name();
    case Qt::UserRole:
        return QVariant::fromValue(group);
    //case StatusRole:
    //    return group->status();
    }

    return QVariant();
}

void GroupsModel::setManager(GroupsManager *manager)
{
    if (manager == m_manager)
        return;
    beginResetModel();
    foreach (Group *group, m_groups)
        group->disconnect(this);
    m_groups.clear();
    if (m_manager)
    {
        disconnect(m_manager, 0, this, 0);
        //disconnect(m_manager->groups, 0, this, 0);
    }
    m_manager = manager;

    if (m_manager)
    {
        connect(manager, SIGNAL(groupAdded(Group*)), SLOT(groupAdded(Group*)));
        m_groups = manager->groups;
        std::sort(m_groups.begin(), m_groups.end(), groupSort);
        foreach (Group *group, m_groups)
            connectSignals(group);
    }
    endResetModel();
    emit managerChanged();
}

void GroupsModel::connectSignals(Group *group)
{
    connect(group, SIGNAL(nameChanged()), SLOT(updateGroup()));
}

void GroupsModel::groupAdded(Group *group)
{
    Q_ASSERT(!indexOfGroup(group).isValid());
    connectSignals(group);

    QList<Group*>::Iterator lp = qLowerBound(m_groups.begin(), m_groups.end(), group, groupSort);
    int row = lp - m_groups.begin();
    beginInsertRows(QModelIndex(), row, row);
    m_groups.insert(lp, group);
    endInsertRows();
}


void GroupsModel::testSendMessage(Group *group) const
{
    group->testSendMessage();
}
