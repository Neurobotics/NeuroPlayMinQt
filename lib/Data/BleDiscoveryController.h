#ifndef BLEDISCOVERYCONTROLLER_H
#define BLEDISCOVERYCONTROLLER_H

#include <QObject>

#include "Core_global.h"
#include "BleDiscovery.h"
#include "helpers/ConnectionHelper.h"

class CORE_EXPORT BleDiscoveryController : public QObject
{
    Q_OBJECT
public:
    BleDiscoveryController(QObject *parent = nullptr);
    ~BleDiscoveryController();

    bool start(int TimeOut = BLE_DISCOVERY_TIMEOUT);
    void stop();
    QThread* m_thread = nullptr;

    void connectToDevice(QString name);

signals:
    void addDevice(const QBluetoothDeviceInfo &in_device);
    void deviceConnected(const QBluetoothDeviceInfo &in_device);
    void discoveryFinished();

public slots:
    void onAddDevice(const QBluetoothDeviceInfo &in_device);
    void onDiscoveryFinished();

protected:
    BleDiscovery* m_bleDiscovery = nullptr;
    ConnectionHelper m_connections;
};

#endif // BLEDISCOVERYCONTROLLER_H
