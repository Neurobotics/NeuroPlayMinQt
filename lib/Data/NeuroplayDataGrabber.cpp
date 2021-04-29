#include "NeuroplayDataGrabber.h"

NeuroplayDataGrabber::NeuroplayDataGrabber(int realChannels)
{
    m_realChannels = realChannels;
}

 QVector<QVector<float>> NeuroplayDataGrabber::addData(QByteArray value)
{
    QVector<float> samples;

    const int frameCount = 4;
    const int frameDataSize = 18;
    const int frameSize = 20;
    const int frameDataPos = frameSize - frameDataSize;

    const int packetDataSize = frameCount * frameDataSize;

    const int sampleDataSize = 3;
    const int sampleChannelCount = 8;

    if (value.size() == frameSize)
        m_InputQueue.enqueue(value);



    if (m_InputQueue.size() >= frameCount)
    {


        bool is_frame = false;
        char frame_number = m_InputQueue[0][0];
        if (static_cast<char>(m_InputQueue[1][0]) == frame_number + 1 &&
                static_cast<char>(m_InputQueue[2][0]) == frame_number + 2 &&
                static_cast<char>(m_InputQueue[3][0]) == frame_number + 3)
        {
            is_frame = true;

            QVector<QVector<float>> channelWiseData;
            for (int i = 0; i<m_realChannels; i++)
            {
                channelWiseData << QVector<float>();
            }

            char buf[packetDataSize];
            for (int i = 0; i < frameCount; i++)
            {
                auto bytes = m_InputQueue.dequeue();
                memcpy(&buf[i * frameDataSize], &bytes.constData()[frameDataPos], frameDataSize);
            }

            int sampleCount = packetDataSize / (sampleChannelCount * sampleDataSize);

            if (m_samples.empty() || m_samples.size() < sampleChannelCount)
                m_samples.resize(sampleChannelCount);

            for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
            {
                for (int channelIndex = 0; channelIndex < sampleChannelCount; ++channelIndex)
                {
                    int pos = sampleIndex * (sampleChannelCount * sampleDataSize) + channelIndex * sampleDataSize;
                    int32_t adc = 0;
                    char *p_adc = reinterpret_cast<char*>(&adc);
                    for (int byteIndex = 0; byteIndex < sampleDataSize; ++byteIndex)
                    {
                        p_adc[sampleDataSize-byteIndex] = buf[pos + byteIndex];
                    }

                    double val = microVoltsInBit * adc;
                    m_samples.replace(channelIndex, val);
                }

                switch (m_realChannels)
                {
                    case 1:
                    {
                        channelWiseData[0] << m_samples;
                    }
                    break;

                    case 2:
                    {
                        channelWiseData[0] << m_samples[0];
                        channelWiseData[1] << m_samples[1];
                    }
                    break;

                    case 4:
                    {
                        channelWiseData[0] << m_samples[0];
                        channelWiseData[1] << m_samples[3];
                        channelWiseData[2] << m_samples[4];
                        channelWiseData[3] << m_samples[7];
                        //samples << QVector<float> { m_samples[0],  m_samples[3], m_samples[4],  m_samples[7] };
                    }
                    break;

                    case 6:
                    {
                        channelWiseData[0] << m_samples[0];
                        channelWiseData[1] << m_samples[2];
                        channelWiseData[2] << m_samples[3];
                        channelWiseData[3] << m_samples[4];
                        channelWiseData[4] << m_samples[5];
                        channelWiseData[5] << m_samples[7];

                        samples << QVector<float> { m_samples[0], m_samples[2], m_samples[3], m_samples[4], m_samples[5], m_samples[7] };
                    }
                    break;

                    case 8:
                    {
                        channelWiseData[0] << m_samples[0];
                        channelWiseData[1] << m_samples[1];
                        channelWiseData[2] << m_samples[2];
                        channelWiseData[3] << m_samples[3];
                        channelWiseData[4] << m_samples[4];
                        channelWiseData[5] << m_samples[5];
                        channelWiseData[6] << m_samples[6];
                        channelWiseData[7] << m_samples[7];
                    }
                    break;
                }
            }

            return channelWiseData;
        }

        if (!is_frame)
        m_InputQueue.dequeue();


    }

    return QVector<QVector<float>>();
}
