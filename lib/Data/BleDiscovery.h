#ifndef BLEDISCOVERY_H
#define BLEDISCOVERY_H

#include <QObject>
#include <QLowEnergyController>
#include <QBluetoothDeviceDiscoveryAgent>

#include "Core_global.h"
#include "helpers/ConnectionHelper.h"

#define BLE_DISCOVERY_TIMEOUT (1000 * 15)

class CORE_EXPORT BleDiscovery : public QObject
{
    Q_OBJECT
public:
    BleDiscovery(int TimeOut = BLE_DISCOVERY_TIMEOUT, QObject *parent = nullptr);
    ~BleDiscovery();


    void startConnectDevice(QString name);

signals:
    void addDevice(const QBluetoothDeviceInfo &in_device);
    void finished();
    void deviceConnected(const QBluetoothDeviceInfo &in_device);

public slots:
    void startDiscovery(const int TimeOut = -1);
    void stopDiscovery();

protected slots:
    void deviceDiscovery(const QBluetoothDeviceInfo &in_device);
    void finishedDiscovery();
    void errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error in_err);
    void canceledDiscovery();

protected:
    QBluetoothDeviceDiscoveryAgent* discoveryAgent = nullptr;
    QList<QBluetoothDeviceInfo> m_BluetoothDeviceInfoList;
    QHash<QString, QLowEnergyController*> controllersByName;
    int m_TimeOut = BLE_DISCOVERY_TIMEOUT;

    ConnectionHelper m_connections;

};

#endif // BLEDISCOVERY_H
