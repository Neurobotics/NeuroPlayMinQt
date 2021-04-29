#ifndef BLEDEVICE_H
#define BLEDEVICE_H

#include "Core_global.h"

#include "BleService.h"

#include <QLowEnergyController>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include "helpers/ConnectionHelper.h"

class CORE_EXPORT BleDevice : public QObject
{
    Q_OBJECT
public:
    BleDevice(QObject *parent = nullptr);
    ~BleDevice();

    void startConnectToDevice(const QBluetoothDeviceInfo &bleDevInfo);
    void startDisconnectFromDevice();

    BleService* getBleService(const QBluetoothUuid &uuid);

signals:
    void DiscoveryFinished();

protected slots:

    void deviceConnected();

    void deviceDiscoveryFinished();

    void DiscoveredBleService(const BleService &ble_service);
protected:
    QLowEnergyController* m_QLowEnergyController = nullptr;
    QHash<QBluetoothUuid, BleService*> m_BleServices;
    ConnectionHelper m_connections;
};

#endif // BLEDEVICE_H
