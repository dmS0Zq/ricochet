#include "Group.h"
#include <QCryptographicHash>

QByteArray Group::seedHash() const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(m_seed);
    return hash.result();
}
