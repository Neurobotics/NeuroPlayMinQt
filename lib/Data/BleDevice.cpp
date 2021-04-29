#include "BleDevice.h"

BleDevice::BleDevice(QObject * /*parent*/)
{
    m_QLowEnergyController = nullptr;
}

BleDevice::~BleDevice()
{
    m_connections.clear();

    if (m_QLowEnergyController)
    {
        m_QLowEnergyController->disconnected();
        delete m_QLowEnergyController;
        m_QLowEnergyController = nullptr;
    }
}

void BleDevice::startConnectToDevice(const QBluetoothDeviceInfo& bleDevInfo)
{
    if (m_QLowEnergyController)
        if (m_QLowEnergyController->state() != QLowEnergyController::UnconnectedState)
            return;

    m_QLowEnergyController = new QLowEnergyController(bleDevInfo, this);

    m_connections << connect(m_QLowEnergyController, &QLowEnergyController::connected, this, &BleDevice::deviceConnected, Qt::QueuedConnection);
    m_connections << connect(m_QLowEnergyController, &QLowEnergyController::discoveryFinished, this, &BleDevice::deviceDiscoveryFinished, Qt::QueuedConnection);

    m_QLowEnergyController->connectToDevice();
}

void BleDevice::startDisconnectFromDevice()
{
    if (!m_QLowEnergyController) return;

   if (m_QLowEnergyController->state() == QLowEnergyController::UnconnectedState)
            return;

    m_QLowEnergyController->disconnectFromDevice();

    // TODO - this invalide all services and char...
}

BleService* BleDevice::getBleService(const QBluetoothUuid &uuid)
{
    BleService* res = m_BleServices.value(uuid, nullptr);
    return res;
}

void BleDevice::deviceConnected()
{
    m_QLowEnergyController->discoverServices();
}

void BleDevice::deviceDiscoveryFinished()
{
    for (QBluetoothUuid uuid : m_QLowEnergyController->services())
    {
        //qDebug() << "DEV" << uuid;

        QLowEnergyService *q_service = m_QLowEnergyController->createServiceObject(uuid);

        if (q_service)
        {
            BleService *ble_service = new BleService(q_service);
            m_connections << connect(ble_service, &BleService::Discovered, this, &BleDevice::DiscoveredBleService);
            m_BleServices.insert(uuid, ble_service);
        }
    }
}

void BleDevice::DiscoveredBleService(const BleService& /*ble_service*/)
{
    bool all_discovered = true;
    foreach (BleService* s, m_BleServices)
    {
        if (s->service()->state() != QLowEnergyService::ServiceDiscovered)
            all_discovered = false;
    }

    if (all_discovered)
        emit DiscoveryFinished();
}
