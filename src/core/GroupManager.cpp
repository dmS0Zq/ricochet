#include "GroupManager.h"

GroupManager *groupManager = 0;

GroupManager::GroupManager(QObject *parent) : QObject(parent)
{
    groupManager = this;
}

GroupManager::~GroupManager()
{
    groupManager = 0;
}
