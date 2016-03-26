#ifndef GROUP_H
#define GROUP_H

#include "ContactUser.h"
#include <QObject>
#include <QMap>

class Group : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Group)
public:
    QByteArray seed() const { return m_seed; }
    QByteArray seedHash() const;
private:
    struct GroupMember
    {
        ContactUser *contactUser;
        QByteArray seed;
        QByteArray seedHash;
    };
    QMap<QByteArray, GroupMember> m_groupMembers;
    QByteArray m_seed;
};

Q_DECLARE_METATYPE(Group*)
#endif // GROUP_H
