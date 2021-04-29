#include "MainWindow.h"
#include "Core.h"
#include <QTextEdit>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto textStatus = new QTextEdit();
    auto textData = new QTextEdit();

    auto centralWidget = new QWidget();
    auto layout = new QHBoxLayout(centralWidget);
    layout->addWidget(textStatus, 100);
    layout->addWidget(textData, 100);

    setCentralWidget(centralWidget);

    auto core = new Core();

    connect(core, &Core::deviceFound, this, [=](DeviceInfo device)
    {
        textStatus->append("FOUND: " + device.name + "\n");

        if (core->currentDeviceInfo().name == "")
        {
            textStatus->append("CONNECTING TO: " + device.name + "\n");
            core->startDevice(device);
        }
    }, Qt::QueuedConnection);

    connect(core, &Core::deviceStarted, this, [=]()
    {
        textStatus->append("CONNECTED TO: " + core->currentDeviceInfo().name + "\n");
    }, Qt::QueuedConnection);

    connect(core, &Core::newData, this, [=](QVector<QVector<float>> channelWiseData)
    {
        QString txt;

        auto channelNames = core->currentDeviceInfo().channels;

        for (int i = 0; i<channelWiseData.length(); i++)
        {
            txt += channelNames[i] + ": ";
            auto channelData = channelWiseData[i];
            for (int j = 0; j<channelData.length(); j++)
            {
                txt +=  QString::number(channelData[j]) + "\t";
            }

            txt += "\n";
        }

        textData->setText(txt);
    }, Qt::QueuedConnection);

    core->startDeviceSearch();
}

MainWindow::~MainWindow()
{
}

