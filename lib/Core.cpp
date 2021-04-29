#include "Core.h"
#include "Data/NeuroplayDevice.h"
#include "Data/BleDiscoveryController.h"
#include <QTimer>
#include <QDateTime>
#include <QCoreApplication>
#include <QStandardPaths>

static QMap<QString, NeuroplayDevice *> neuroplayDevices;
static BleDiscoveryController *bleDiscoveryController = nullptr;

Core::Core(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<DeviceInfo>("DeviceInfo");
    m_dataMutex = new QMutex();
}

Core::~Core()
{
    stopDevice();
    stopDeviceSearch();

    if (bleDiscoveryController)
    {
        bleDiscoveryController->stop();
        delete bleDiscoveryController;
        bleDiscoveryController = nullptr;
    }
}

QList<NeuroplayDevice *> Core::getDeviceList()
{
    return neuroplayDevices.values();
}

bool Core::startDeviceSearch()
{
    if (m_isSearchingDevices) return true;

    qDebug() << ("START DEVICE SEARCH");

    stopDevice();

    m_isSearchingDevices = true;
    emit deviceSearchStarted();

    neuroplayDevices.clear();
    m_foundDevices.clear();

    if (bleDiscoveryController) bleDiscoveryController->stop();

    if (!bleDiscoveryController)
    {
        bleDiscoveryController = new BleDiscoveryController();
//        bleDiscoveryController->moveToThread(qApp->thread());

        m_connections << connect(bleDiscoveryController, &BleDiscoveryController::discoveryFinished, [=]()
        {
            m_isSearchingDevices = false;
            emit deviceSearchFinished();

            if (!m_recconnectToDevice.isEmpty()) startDeviceSearch();
        });

        m_connections << connect(bleDiscoveryController, &BleDiscoveryController::addDevice, [=](const QBluetoothDeviceInfo &in_device)
        {
            auto neuroplayDevice = new NeuroplayDevice(in_device);

            neuroplayDevices[neuroplayDevice->name()] = neuroplayDevice;

            int channels = neuroplayDevice->getChannelCount();

            DeviceInfo deviceInfo;
            deviceInfo.channelCount = channels;
            deviceInfo.model = "Neuroplay";
            deviceInfo.maxChannels = channels;
            deviceInfo.preferredChannelCount = channels;

            switch (channels)
            {
            case 1: deviceInfo.channelToFrequencyMap.insert(1, 1000); break;
            case 2: deviceInfo.channelToFrequencyMap.insert(2, 500);  break;
            case 4: deviceInfo.channelToFrequencyMap.insert(4, 125);  break;
            case 6: deviceInfo.channelToFrequencyMap.insert(6, 125);  break;
            }

            //Add 8 channels anyway
            deviceInfo.channelToFrequencyMap.insert(8, 125);

            QString name = deviceInfo.name = neuroplayDevice->name();
            QString sn = name;

            int posLeftBrace = name.indexOf("(");
            int posRightBrace = name.lastIndexOf(")");

            if (posLeftBrace > 0 && posRightBrace > posLeftBrace + 2)
            {
                sn = name.mid(posLeftBrace +1, posRightBrace-posLeftBrace-1);
            }

            deviceInfo.sn = sn;

            qDebug() << name << sn;

            m_foundDevices << deviceInfo;
            emit deviceFound(deviceInfo);

            if (!m_recconnectToDevice.isEmpty() && deviceInfo.name == m_recconnectToDevice)
            {
                qDebug() << ("RESTART DEVICE " + deviceInfo.name);
                QMetaObject::invokeMethod(this, [=]()
                {
                    startDevice(deviceInfo);
                }, Qt::QueuedConnection);
            }
        });
    }

    m_connections << connect(bleDiscoveryController, &BleDiscoveryController::deviceConnected, this, [=](const QBluetoothDeviceInfo &)
    {
        m_device->isConnected = NeuroplayDevice::ConnectionState::CONNECTED;
    }, Qt::QueuedConnection);

    bleDiscoveryController->start();
    return true;
}

void Core::stopDeviceSearch()
{
    m_isSearchingDevices = false;
    if (bleDiscoveryController)
    {
        bleDiscoveryController->stop();
    }
    emit deviceSearchFinished();
}

int Core::getDeviceCount()
{
    return m_foundDevices.count();
}

void Core::reconnectToDevice(QString deviceName)
{
    QMetaObject::invokeMethod(this, [=]()
    {
        m_recconnectToDevice = deviceName;
        startDeviceSearch();
    }, Qt::QueuedConnection);
}

bool Core::startDevice(DeviceInfo deviceInfo)
{
    m_recconnectToDevice = "";

    if (m_connectingToDevice == deviceInfo.name)
    {
        qDebug() << "ALREADY CONNECTING TO THIS DEVICE" << deviceInfo.name;
        return false;
    }

    stopDevice();

    qDebug() << "START" << deviceInfo.name;

    m_connectingToDevice = deviceInfo.name;

    emit deviceStarting();

    QStringList channelNames;

    if (m_deviceChannels < 0) m_deviceChannels = deviceInfo.preferredChannelCount;

    if (deviceInfo.channelToFrequencyMap.count() == 0) return false;

    m_deviceFrequency = deviceInfo.channelToFrequencyMap[deviceInfo.preferredChannelCount];

    switch(deviceInfo.preferredChannelCount)
    {
        case 1: channelNames = NeuroplayDevice::NP_ChanNames1; break;
        case 2: channelNames = NeuroplayDevice::NP_ChanNames2; break;
        case 4: channelNames = NeuroplayDevice::NP_ChanNames4; break;
        case 6: channelNames = NeuroplayDevice::NP_ChanNames6; break;
        case 8: channelNames = NeuroplayDevice::NP_ChanNames8; break;
    }

    if (m_device)
    {
        disconnect(m_device, nullptr, nullptr, nullptr);

        auto npDev = dynamic_cast<NeuroplayDevice*>(m_device);
        if (npDev) npDev->stopAdc();

        m_device = nullptr;
    }

    if (neuroplayDevices.contains(deviceInfo.name))
        m_device = neuroplayDevices[deviceInfo.name];
    else
        return false;

    deviceInfo.channels = channelNames;

    m_currentDevice = deviceInfo;

    m_deviceConnections << connect(m_device, &NeuroplayDevice::dataReady, this, [=](QVector<QVector<float>> channelWise)
    {
        m_deviceRunning = true;
        //qDebug() << "NEW DATA" << channelWise;

        emit newData(channelWise);

    }, Qt::QueuedConnection);

    m_deviceConnections << connect(m_device, &NeuroplayDevice::disconnected, this, [=]()
    {
        reconnectToDevice(deviceInfo.name);
    }, Qt::QueuedConnection);

    m_deviceConnections << connect(m_device, &NeuroplayDevice::wrongDataRate, this, [=]()
    {
        reconnectToDevice(deviceInfo.name);
    }, Qt::QueuedConnection);


    QEventLoop loop;
    while (true)
    {
        if (!m_device) break;

        if (m_device->isConnected == 0)
        {
            auto np8gDevice = dynamic_cast<NeuroplayDevice*>(m_device);

            static bool dRes = false;
            static bool dStarted = false;

            dRes = false;
            dStarted = false;

            QMetaObject::invokeMethod(np8gDevice, [=]()
            {
                np8gDevice->open();
                dStarted = np8gDevice->startAdc(static_cast<int>(m_deviceFrequency));
                dRes = true;
            }, Qt::QueuedConnection);

            while (!dRes)
            {
                loop.processEvents(QEventLoop::AllEvents, 100);
            }

            if (dStarted || np8gDevice->getIsVirtualFlag())
            {
                qDebug() << "Dev started";
                emit deviceStarted();
                m_currentDevice.batteryLevel = np8gDevice->getCurrentBatteryLevel();

                m_deviceConnections << connect(np8gDevice, &NeuroplayDevice::batteryLevelUpdated, [=](int value)
                {
                    m_currentDevice.batteryLevel = value;
                    emit deviceBatteryLevelUpdated(value);
                });

                emit deviceStartedInfo(m_currentDevice);
                break;
            }
        }
        else if (m_device->isConnected == NeuroplayDevice::ConnectionState::NOT_CONNECTED)
        {
            bleDiscoveryController->connectToDevice(m_device->name());
            m_device->isConnected = NeuroplayDevice::ConnectionState::AWAITING_CONNECTION;
        }
        else
        {
            loop.processEvents(QEventLoop::AllEvents, 100);
        }
    }

    return true;
}

void Core::stopDevice()
{
    m_deviceConnections.clear();

    if (m_device)
    {
        m_device->stopAdc();
        m_device->close();
        m_deviceRunning = false;

        m_currentDevice = DeviceInfo();
        emit deviceStopped();

        m_device = nullptr;

        delete m_device;

        m_connectingToDevice = "";
    }
}

DeviceInfo Core::currentDeviceInfo()
{
    return m_currentDevice;
}

QList<DeviceInfo> Core::getFoundDevices()
{
    return m_foundDevices;
}
