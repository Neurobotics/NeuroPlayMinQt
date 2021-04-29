#include "BleDiscovery.h"
#include "NeuroplayDevice.h"

static bool isRegisterMetaType = false;

BleDiscovery::BleDiscovery(int TimeOut, QObject *parent) : QObject(parent)
{
    m_TimeOut = TimeOut;

    if (!isRegisterMetaType)
    {
        isRegisterMetaType = true;

        qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");
        qRegisterMetaType<QBluetoothDeviceDiscoveryAgent::Error>("QBluetoothDeviceDiscoveryAgent::Error");
        qRegisterMetaType<QLowEnergyService::ServiceError>("QLowEnergyService::ServiceError");
        qRegisterMetaType<QLowEnergyService::ServiceState>();
        qRegisterMetaType<QLowEnergyService::ServiceType>();
        qRegisterMetaType<QLowEnergyService::WriteMode>();
        qRegisterMetaType<QLowEnergyCharacteristic>();
        qRegisterMetaType<QLowEnergyDescriptor>();
    }
}

BleDiscovery::~BleDiscovery()
{
    m_connections.clear();

    for(auto& controller : controllersByName)
    {
        controller->disconnectFromDevice();
        delete controller;
    }

    //qDeleteAll(controllersByName);

    controllersByName.clear();

    if (discoveryAgent)
    {
        discoveryAgent->stop();
//        delete discoveryAgent;
//        discoveryAgent = nullptr;
    }
}

void BleDiscovery::startConnectDevice(QString name)
{
    QBluetoothDeviceInfo in_device;

    bool found = false;
    foreach(auto dev, m_BluetoothDeviceInfoList)
    {
        if (dev.name() == name)
        {
            in_device = dev;
            found = true;
        }
    }

    if (!found) return;

//    if(!in_device.serviceUuids().isEmpty())
//    {
//        qDebug() << "Already has services!";
//        emit addDevice(in_device);
//        return;
//    }

    auto controller = QLowEnergyController::createCentral(in_device);

    controllersByName[in_device.name()] = controller;

    m_connections << connect(controller, &QLowEnergyController::connected, this, [=]()
    {
        controller->discoverServices();
    });
    m_connections << connect(controller, &QLowEnergyController::discoveryFinished, this, [=]()
    {
        qDebug() << ("Service discovery finished for " + in_device.name());
        emit deviceConnected(in_device);
    });
    m_connections << connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, [=](QLowEnergyController::Error )
    {
        qDebug() << ("Error! " + controller->errorString() + "Rediscovering...");
        controller->discoverServices();
    });
    m_connections << connect(controller, &QLowEnergyController::serviceDiscovered, this, [=](const QBluetoothUuid& /*uuid*/)
    {
//        if(uuid == NeuroplayDevice::BLUETOOTH_UUID_EEG)
//        {
//            m_BluetoothDeviceInfoList.push_back(in_device);
//        }
    });



    qDebug() << ("Connecting to device... " + in_device.name());
    controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    controller->connectToDevice();
}

void BleDiscovery::startDiscovery(const int TimeOut)
{
    qDebug() << ("BLE Start discovery");

    if (!discoveryAgent)
    {
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
        discoveryAgent->setInquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry);

        m_connections << connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovery(QBluetoothDeviceInfo)), Qt::QueuedConnection);
        m_connections << connect(discoveryAgent, SIGNAL(finished()), this, SLOT(finishedDiscovery()),  Qt::QueuedConnection);
        m_connections << connect(discoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error)), Qt::QueuedConnection);
        m_connections << connect(discoveryAgent, SIGNAL(canceled()), this, SLOT(canceledDiscovery()),  Qt::QueuedConnection);
    }

    if (TimeOut >= 0)
    {
        m_TimeOut = TimeOut;
    }

    if (discoveryAgent)
    {
        discoveryAgent->setLowEnergyDiscoveryTimeout(m_TimeOut);
        discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }
}

void BleDiscovery::stopDiscovery()
{
    qDebug() << ("BLE Stop discovery");

    if (discoveryAgent && discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }
}

void BleDiscovery::deviceDiscovery(const QBluetoothDeviceInfo& in_device)
{
    if(in_device.name().isEmpty()) return;
    if(!in_device.name().startsWith("neuroplay", Qt::CaseInsensitive) && !in_device.name().startsWith("np", Qt::CaseInsensitive) && !in_device.name().startsWith("Physiobelt", Qt::CaseInsensitive)) return;

    if (!m_BluetoothDeviceInfoList.contains(in_device))
    {
//#ifdef Q_OS_MAC
//        // On OS X and iOS we do not have addresses,
//        // only unique UUIDs generated by Core Bluetooth.
//        in_device.deviceUuid().toString();
//#else
//        in_device.address().toString();
//#endif
        m_BluetoothDeviceInfoList.push_back(in_device);
        emit addDevice(in_device);
    }
}



void BleDiscovery::finishedDiscovery()
{
    emit finished();
}

void BleDiscovery::errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error /*in_err*/)
{

}

void BleDiscovery::canceledDiscovery()
{

}
