#include "BleService.h"

BleService::BleService(QLowEnergyService *service)
{
    m_QLowEnergyService = service;

    m_connections << connect(m_QLowEnergyService, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(stateChanged(QLowEnergyService::ServiceState)), Qt::QueuedConnection);
    m_connections << connect(m_QLowEnergyService, &QLowEnergyService::characteristicRead, this, &BleService::characteristicRead, Qt::QueuedConnection);
    m_connections << connect(m_QLowEnergyService, &QLowEnergyService::characteristicChanged, this, &BleService::characteristicChanged);

    m_QLowEnergyService->discoverDetails(); // TODO - async
}

QLowEnergyService *BleService::service()
{
    return m_QLowEnergyService;
}

bool BleService::startReadCharacteristic(const QBluetoothUuid &uuid)
{
    const QLowEnergyCharacteristic hrChar = m_QLowEnergyService->characteristic(uuid);
    if (hrChar.isValid())
        m_QLowEnergyService->readCharacteristic(hrChar);

    return false;
}

bool BleService::WriteCharacteristic(const QBluetoothUuid &uuid, const QByteArray &value)
{
    const QLowEnergyCharacteristic hrChar = m_QLowEnergyService->characteristic(uuid);

    if (hrChar.isValid())
        m_QLowEnergyService->writeCharacteristic(hrChar, value); //, QLowEnergyService::WriteWithoutResponse);

    return false;
}

bool BleService::NotificationEnable(const QBluetoothUuid &uuid, bool enable)
{
    const QLowEnergyCharacteristic ble_characteristic = m_QLowEnergyService->characteristic(uuid);

    if (!ble_characteristic.isValid())
        return false;

    const QLowEnergyDescriptor ble_descriptor =
            ble_characteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);

    if (!ble_descriptor.isValid())
        return false;

    if (enable)
        m_QLowEnergyService->writeDescriptor(ble_descriptor, QByteArray::fromHex("0100"));
    else
        m_QLowEnergyService->writeDescriptor(ble_descriptor, QByteArray::fromHex("0000"));

    return true;
}

void BleService::stateChanged(QLowEnergyService::ServiceState state)
{
    //qDebug() << "SERVICE STATE" << m_QLowEnergyService->serviceUuid() << state;

    switch (state)
    {
    case QLowEnergyService::ServiceDiscovered:
        emit Discovered(*this);
        break;
    case QLowEnergyService::DiscoveringServices:
        break;
    default:
        break;
    }
}

void BleService::characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (value.isEmpty())
        return;

    QString s;
    for (int i = 0, sz = value.size(); i < sz; i++)
    {
        ushort d = static_cast<ushort>(value[i]);
        s.append(QString("0x%1 ").arg(d, 2, 16));
    }
    emit signalCharacteristicRead(characteristic, value);
}

void BleService::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
#if 0
    QString s;
    s = value.size() <= 20 ? value.toHex(' ') : "";

    qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz")
             << "BleService::characteristicChanged" << characteristic.uuid()
             << '(' << value.size() << " bytes : " << s << ')';
#endif
    emit NotificationData(characteristic.uuid(), value);
}

