#ifndef CONNECTIONHELPER_H
#define CONNECTIONHELPER_H

#include <QObject>
#include <QMetaObject>
#include "Core_global.h"

class CORE_EXPORT ConnectionHelper
{
public:
    ConnectionHelper();
    ~ConnectionHelper();

    void operator << (QMetaObject::Connection connection);

    void clear();

    QList<QMetaObject::Connection> m_connections;

    static void clear(QList<QMetaObject::Connection> &connections);
};

#endif // CONNECTIONHELPER_H
