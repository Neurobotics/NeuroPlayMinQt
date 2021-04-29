#ifndef NEUROPLAYDATAGRABBER_H
#define NEUROPLAYDATAGRABBER_H

#include "Core_global.h"
#include <QVector>
#include <QQueue>

class CORE_EXPORT NeuroplayDataGrabber
{
public:
    NeuroplayDataGrabber(int realChannels);

    QVector<QVector<float>> addData(QByteArray value);

    int m_realChannels = 0;

protected:
    QVector<float> m_samples;
    QQueue<QByteArray> m_InputQueue;
    QQueue<QVector<float>> m_OutputQueue;

    const double microVoltsInBit = (2 * 2.4 / 6) / (1 << 24) / 256 * 1e6;
    const double m_VoltInBit = (2 * 2.4 / 6) / (1 << 24); // TODO - GAIN = 6
};
#endif // NEUROPLAYDATAGRABBER_H
