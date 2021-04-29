#ifndef NEUROPLAYDEVICE_H
#define NEUROPLAYDEVICE_H

#include "Core_global.h"
#include "BleService.h"
#include "BleDevice.h"

#include <QObject>
#include <QBluetoothUuid>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyCharacteristic>
#include "helpers/ConnectionHelper.h"
#include <QTimer>

class CORE_EXPORT NeuroplayDevice : public QObject
{
    Q_OBJECT
public:
    enum ConnectionState
    {
        CONNECTED,
        NOT_CONNECTED,
        AWAITING_CONNECTION
    };

    static QBluetoothUuid BLUETOOTH_UUID_EEG;
    static QBluetoothUuid BLUETOOTH_UUID_EEGDATA;
    static QBluetoothUuid BLUETOOTH_UUID_EEGCONTROL;

    int m_hardwareChannels = 8;
    int m_realChannels = 8;
    int m_resultChannels = 8;

    ConnectionState isConnected = NOT_CONNECTED;

    NeuroplayDevice(const QBluetoothDeviceInfo& in_device);
    NeuroplayDevice();
    ~NeuroplayDevice();

    QBluetoothDeviceInfo bluetoothDeviceInfo() const;

    int getChannelCount() { return m_realChannels; }

    virtual void open();
    virtual void close();

    bool startAdc(int rate);
    bool stopAdc();

    int getCurrentBatteryLevel();

    QString name() const;
    bool getIsVirtualFlag() { return m_isVirtualDevice; }


signals:
    void bleReady(NeuroplayDevice *device);
    void dataReady(QVector<QVector<float>> channelWise);

    void disconnected();
    void wrongDataRate();

    void batteryLevelUpdated(int value);

protected slots:
    void discoveryFinished();
    void slotCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

protected:
    bool startReadBattery();

    QBluetoothDeviceInfo m_bleDevInfo;

    BleDevice* m_bleDevice = nullptr;
    bool m_isVirtualDevice = false;

    BleService* m_bleServiceEeg = nullptr;
    BleService* m_bleServiceBattery = nullptr;

    int m_rate = 125;
    int m_batteryLevel = -1;
    QString m_deviceName ="";

    ConnectionHelper m_connections;

    QTimer *m_connectionTimer = nullptr;
    qint64 m_lastDataTimestamp = 0;
    qint64 m_packetsReceived = 0;
    int m_wrongDataRateCounter = 0;
    int m_wrongDataRateCounterMax = 5;
    int m_connectionWatchCycles = 0;
    int m_lowRateWatchCyclesMax = 100;

public:
    static QStringList NP_ChanNames1;
    static QStringList NP_ChanNames2;
    static QStringList NP_ChanNames4;
    static QStringList NP_ChanNames6;
    static QStringList NP_ChanNames8;
    static QBluetoothUuid getUuidEeg();
    static void setUuidEeg(const QBluetoothUuid &UuidEeg);

};

#endif // NEUROPLAYDEVICE_H
