#include <QLowEnergyService>
#include <QLowEnergyController>

#include "NeuroplayDevice.h"
#include "NeuroplayDataGrabber.h"
#include <QDateTime>

QStringList NeuroplayDevice::NP_ChanNames1 = { "O1" };
QStringList NeuroplayDevice::NP_ChanNames2 = { "O1", "O2" };
QStringList NeuroplayDevice::NP_ChanNames4 = { "O1", "F3", "F4", "O2" };
QStringList NeuroplayDevice::NP_ChanNames6 = { "O1", "T3","F3", "F4", "T4", "O2" };
QStringList NeuroplayDevice::NP_ChanNames8 = { "O1", "P3", "C3", "F3", "F4", "C4", "P4", "O2"};

QBluetoothUuid NeuroplayDevice::BLUETOOTH_UUID_EEG = QBluetoothUuid::fromString(QLatin1String("{f0001298-0451-4000-b000-000000000000}"));
QBluetoothUuid NeuroplayDevice::BLUETOOTH_UUID_EEGDATA = QBluetoothUuid::fromString(QLatin1String("{f0001299-0451-4000-b000-000000000000}"));
QBluetoothUuid NeuroplayDevice::BLUETOOTH_UUID_EEGCONTROL = QBluetoothUuid::fromString(QLatin1String("{f000129a-0451-4000-b000-000000000000}"));

NeuroplayDevice::NeuroplayDevice(const QBluetoothDeviceInfo& in_device)
{
    m_bleDevInfo = in_device;
    m_deviceName = m_bleDevInfo.name();

    QString sn = m_bleDevInfo.name();

    //qDebug() << "SN" << sn;

    if (sn.indexOf("NP8G (2C") >= 0) // TODO - via command
    {
        m_hardwareChannels = 2;
        m_realChannels = 2;
    }
    else if (sn.startsWith("NeuroPlay-4", Qt::CaseInsensitive) || sn.startsWith("NeuroPlay4", Qt::CaseInsensitive) || sn.startsWith("NP4", Qt::CaseInsensitive) || sn.startsWith("NG4", Qt::CaseInsensitive))
    {
        m_realChannels = 4;
        m_hardwareChannels = 8;
    }
    else if (sn.startsWith("NeuroPlay-6", Qt::CaseInsensitive) || sn.startsWith("NeuroPlay6", Qt::CaseInsensitive) || sn.startsWith("NP6", Qt::CaseInsensitive) || sn.startsWith("NG6", Qt::CaseInsensitive))
    {
        m_realChannels = 6;
        m_hardwareChannels = 8;
    }

    m_bleDevice = new BleDevice(this);
    m_connections << connect(m_bleDevice, &BleDevice::DiscoveryFinished, this, &NeuroplayDevice::discoveryFinished, Qt::QueuedConnection);
}

NeuroplayDevice::NeuroplayDevice()
{
    m_isVirtualDevice = true;
    emit bleReady(this);
}

NeuroplayDevice::~NeuroplayDevice()
{
    m_connections.clear();

    if (m_connectionTimer)
    {
        m_connectionTimer->stop();
    }
    delete m_bleDevice;
}

QBluetoothDeviceInfo NeuroplayDevice::bluetoothDeviceInfo() const
{
    return m_bleDevInfo;
}

void NeuroplayDevice::open()
{
    if (m_bleDevice)
    m_bleDevice->startConnectToDevice(m_bleDevInfo);
}

void NeuroplayDevice::close()
{
    if (m_bleDevice)
    m_bleDevice->startDisconnectFromDevice();
}

void NeuroplayDevice::discoveryFinished()
{
    m_bleServiceEeg = m_bleDevice->getBleService(BLUETOOTH_UUID_EEG);
    m_connections << connect(m_bleServiceEeg, &BleService::NotificationData, this, [=](const QBluetoothUuid &, const QByteArray &value)
    {
        static NeuroplayDataGrabber *grabber = nullptr;
        if (grabber == nullptr || grabber->m_realChannels != m_resultChannels)
        {
            grabber = new NeuroplayDataGrabber(m_resultChannels);
        }

        QVector<QVector<float>> sample = grabber->addData(value);
        if (sample.length() > 0)
        {
            emit dataReady(sample);
        }

        m_lastDataTimestamp = QDateTime::currentMSecsSinceEpoch();
        m_packetsReceived ++;
    }, Qt::QueuedConnection);

    m_bleServiceBattery = m_bleDevice->getBleService(QBluetoothUuid(QBluetoothUuid::BatteryService));
    m_connections << connect(m_bleServiceBattery, SIGNAL(signalCharacteristicRead(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(slotCharacteristicRead(QLowEnergyCharacteristic,QByteArray)), Qt::QueuedConnection);

    startReadBattery(); //start read battery async
    emit bleReady(this);
}

void NeuroplayDevice::slotCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == QBluetoothUuid(QBluetoothUuid::BatteryLevel))
    {
        qDebug() << ("BATTERY " + QString::number(int(value[0])) + " " + name());
        m_batteryLevel = static_cast<unsigned char>(value[0]);
        emit batteryLevelUpdated(value[0]);
    }
}

int NeuroplayDevice::getCurrentBatteryLevel()
{
    return m_batteryLevel;
}

bool NeuroplayDevice::startAdc(int rate)
{
    bool res = false;

    startReadBattery(); // start read battery async

    if (m_bleServiceEeg)
    {
        m_bleServiceEeg->NotificationEnable(BLUETOOTH_UUID_EEGDATA, true);
        switch(rate)
        {
        case 125:
        case 250:
        case 500:
        case 1000:
            m_resultChannels = 1000 / rate; // TODO
            break;
        default:
            m_resultChannels = 8;
            rate = 125;
            break;
        }

        if (m_resultChannels > m_realChannels)
        {
            m_resultChannels = m_realChannels;
        }

        m_rate = rate;
        const char raw[] = {1, static_cast<char>(rate / 125) };
        m_bleServiceEeg->WriteCharacteristic(BLUETOOTH_UUID_EEGCONTROL,  QByteArray(raw, sizeof(raw)));
        res = true;

        m_lastDataTimestamp = QDateTime::currentMSecsSinceEpoch();
        m_packetsReceived = 0;

        if (!m_connectionTimer)
        {
            m_connectionTimer = new QTimer();
            m_connectionTimer->setInterval(1000);

            static int awaitBatteryCycles = 0;
            static int awaitBatteryCyclesMax = 60;

            m_connections << connect(m_connectionTimer, &QTimer::timeout, this, [=]()
            {
                qint64 dt = QDateTime::currentMSecsSinceEpoch();
                if (dt - m_lastDataTimestamp > 3000)
                {
                    qDebug() << "NEUROPLAY DISCONNECTED";
                    QMetaObject::invokeMethod(this, [=]()
                    {
                        stopAdc();
                        emit disconnected();
                    }, Qt::QueuedConnection);
                    m_connectionTimer->stop();
                }
                else
                {
                    double packetsPerTime = (3*m_packetsReceived/4)/(m_connectionTimer->interval()/1000.0);
                    double packetRatio = packetsPerTime/m_rate;

                    m_connectionWatchCycles ++;

                    //qDebug() << ("Packets " + QString::number(m_packetsReceived) + " per " +  QString::number(m_connectionTimer->interval()) + " ms gives "+ QString::number(packetsPerTime) + " per second, ratio is " + QString::number(packetRatio));

                    if (m_connectionWatchCycles < m_lowRateWatchCyclesMax)
                    {
                        if (packetRatio > 0.3)
                        {
                            if (packetRatio < 0.59 || packetRatio > 1.3)
                            {
                                qDebug() << ("KINDA WRONG DATA RATE " + QString::number(packetRatio));
                                m_wrongDataRateCounter++;
                            }
                            else
                            {
                                m_wrongDataRateCounter--;
                                if (m_wrongDataRateCounter < 0)
                                {
                                    m_wrongDataRateCounter = 0;
                                }
                            }

                            if (m_wrongDataRateCounter == m_wrongDataRateCounterMax)
                            {
                                emit wrongDataRate();
                                qDebug() << ("WRONG DATA RATE!!!");
                            }
                        }
                    }
                    else if (m_connectionWatchCycles == m_lowRateWatchCyclesMax)
                    {
                        qDebug() << ("From now on bad frame rate is not watched");
                    }
                    m_packetsReceived = 0;

                    awaitBatteryCycles ++;
                    if (awaitBatteryCycles >= awaitBatteryCyclesMax)
                    {
                        awaitBatteryCycles = 0;
                        qDebug() << ("Reading battery");
                        startReadBattery();
                    }
                }
            }, Qt::QueuedConnection);
        }

        m_connectionTimer->start();
    }
    return res;
}

bool NeuroplayDevice::stopAdc()
{
    bool res = false;
    if (m_bleServiceEeg)
    {
        m_bleServiceEeg->WriteCharacteristic(BLUETOOTH_UUID_EEGCONTROL, QByteArray::fromHex("00"));
        m_bleServiceEeg->NotificationEnable(BLUETOOTH_UUID_EEGDATA, false);
        res = true;
    }

    //startReadBattery(); // start read battery async

    if (m_connectionTimer)
    {
        QMetaObject::invokeMethod(this, [=]()
        {
            m_connectionTimer->stop();
        }, Qt::QueuedConnection);
    }

    return res;
}

bool NeuroplayDevice::startReadBattery()
{
    bool res = false;
    if (m_bleServiceBattery)
    {
        res = m_bleServiceBattery->startReadCharacteristic(QBluetoothUuid(QBluetoothUuid::BatteryLevel));
        //GlobalConsole::writeItem("Battery is " +QString::number(res), name());
    }
    else
    {
        //qDebug() << "NO BATTERY SERVICE";
    }

    return res;
}

QBluetoothUuid NeuroplayDevice::getUuidEeg()
{
    return BLUETOOTH_UUID_EEG;
}

void NeuroplayDevice::setUuidEeg(const QBluetoothUuid &UuidEeg)
{
    BLUETOOTH_UUID_EEG = UuidEeg;
}

QString NeuroplayDevice::name() const
{
    return m_deviceName;
}
