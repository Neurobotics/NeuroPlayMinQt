#include "ConnectionHelper.h"

ConnectionHelper::ConnectionHelper()
{

}

ConnectionHelper::~ConnectionHelper()
{
    clear();
}

void ConnectionHelper::operator <<(QMetaObject::Connection connection)
{
    m_connections << connection;
}

void ConnectionHelper::clear()
{
    clear(m_connections);
}

void ConnectionHelper::clear(QList<QMetaObject::Connection> &connections)
{
    foreach (auto connection, connections)
    {
        QObject::disconnect(connection);
    }
    connections.clear();
}
