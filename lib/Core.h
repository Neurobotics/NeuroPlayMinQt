#ifndef CORE_H
#define CORE_H

#include <QElapsedTimer>
#include <QObject>
#include <QDebug>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QFile>

#include "helpers/ConnectionHelper.h"
#include "Data/NeuroplayDataGrabber.h"

class NeuroplayDevice;

class CORE_EXPORT DeviceInfo
{
public:
    QString name = "";
    QString version = "";
    QString sn = "";
    QString model = "";
    int maxChannels = 0;
    int preferredChannelCount = 8;
    int channelCount = 0;
    int batteryLevel = 0;
    QMap<int, float> channelToFrequencyMap;
    QStringList channels;
};

class CORE_EXPORT Core : public QObject
{
    Q_OBJECT
public:
    Core(QObject* parent = nullptr);
    virtual ~Core();

    //Devices
    bool startDeviceSearch();
    void stopDeviceSearch();

    QList<NeuroplayDevice *> getDeviceList();

    int getDeviceCount();
    DeviceInfo getDevice(int index);

    bool startDevice(DeviceInfo device);
    void stopDevice();

    DeviceInfo currentDeviceInfo();

    QList<DeviceInfo> getFoundDevices();
    DeviceInfo getCurrentDevice() { return m_currentDevice; }

signals:
    //Devices
    void deviceFound(DeviceInfo info);
    void deviceLost(DeviceInfo info);
    void deviceSearchStarted();
    void deviceSearchFinished();

    void deviceStarting();
    void deviceStarted();
    void deviceStartedInfo(DeviceInfo info);
    void deviceStopped();
    void deviceBatteryLevelUpdated(int value);

    void newData(QVector<QVector<float>> data);

protected:
    QList<DeviceInfo> m_foundDevices;    

    QString m_lastFilename = "";

    bool m_deviceRunning = false;
    bool m_isSearchingDevices = false;

    QString m_connectingToDevice = "";
    QString m_dataFolder = "";

    int m_deviceChannels = 0;
    float m_deviceFrequency = 0;

    NeuroplayDevice *m_device = nullptr;
    DeviceInfo m_currentDevice;

    QMutex *m_dataMutex = nullptr;

    QString m_recconnectToDevice = "";

    void reconnectToDevice(QString deviceName);

    ConnectionHelper m_connections;
    ConnectionHelper m_deviceConnections;
    ConnectionHelper m_virtualDeviceConnections;
};

#endif // CORE_H
