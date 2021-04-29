#include "BleDiscoveryController.h"
#include <QThread>

BleDiscoveryController::BleDiscoveryController(QObject* parent) : QObject (parent)
{

}

BleDiscoveryController::~BleDiscoveryController()
{
    stop();
}

bool BleDiscoveryController::start(int TimeOut)
{
    if (m_thread) { return false; }

    qDebug() << "BLE Create thread";
    m_thread = new QThread();

    if (m_bleDiscovery)
    {
        m_bleDiscovery->stopDiscovery();
        delete m_bleDiscovery;
        m_bleDiscovery = nullptr;
    }

    m_bleDiscovery = new BleDiscovery(TimeOut);
    m_bleDiscovery->moveToThread(m_thread);

    m_connections << connect(m_thread, SIGNAL(started()), m_bleDiscovery, SLOT(startDiscovery()));
    m_connections << connect(m_thread, SIGNAL(finished()), m_bleDiscovery, SLOT(stopDiscovery()));

    m_connections << connect(m_bleDiscovery, &BleDiscovery::addDevice, [=](const QBluetoothDeviceInfo &x)
    {
        onAddDevice(x);
    });

    m_connections << connect(m_bleDiscovery, &BleDiscovery::deviceConnected, [=](const QBluetoothDeviceInfo &x)
    {
        emit deviceConnected(x);
    });

    m_connections << connect(m_bleDiscovery, SIGNAL(finished()), this, SLOT(onDiscoveryFinished()), Qt::QueuedConnection);

    m_thread->start();

    return true;
}

void BleDiscoveryController::stop()
{
    if (m_bleDiscovery)
    {
        m_bleDiscovery->stopDiscovery();
        delete m_bleDiscovery;
        m_bleDiscovery = nullptr;
    }

    if (m_thread)
    {
        if (!m_thread->isFinished())
        {
            m_thread->quit();

            // wait up to 10 sec
            for (int i = 0; i < 100; i++)
            {
                QThread::currentThread()->msleep(100);

                if (m_thread->isFinished())
                    break;
            }
            if (!m_thread->isFinished())
                m_thread->terminate();
        }
        delete m_thread;
        m_thread = nullptr;
    }
}

void BleDiscoveryController::connectToDevice(QString name)
{
    if (m_bleDiscovery)
    {
        m_bleDiscovery->startConnectDevice(name);
    }
}

void BleDiscoveryController::onAddDevice(const QBluetoothDeviceInfo &in_device)
{
    emit addDevice(in_device);
}

void BleDiscoveryController::onDiscoveryFinished()
{
    emit discoveryFinished();
}
