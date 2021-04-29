#ifndef BLESERVICE_H
#define BLESERVICE_H

#include <QLowEnergyService>
#include "helpers/ConnectionHelper.h"

class BleService : public QObject
{
    Q_OBJECT
public:
    BleService(QLowEnergyService* service);

    QLowEnergyService* service();

    bool startReadCharacteristic(const QBluetoothUuid &uuid); //, QByteArray *value);
    bool WriteCharacteristic(const QBluetoothUuid &uuid, const QByteArray &value);

    bool NotificationEnable(const QBluetoothUuid &uuid, bool enable);

signals:
    void signalCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    void NotificationData(const QBluetoothUuid &uuid, const QByteArray &value);
    void Discovered(const BleService &ble_service);

protected slots:
    void stateChanged(QLowEnergyService::ServiceState state);

    void characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

protected:
    QLowEnergyService* m_QLowEnergyService = nullptr;
    ConnectionHelper m_connections;
};


#endif // BLESERVICE_H
