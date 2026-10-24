#include "FrameParser.h"

bool FrameParser::feed(const QByteArray& data, QList<EngineFrame>& frames)
{
    m_buffer.append(data);
    while (true) {
        if (m_payloadSize < 0) {
            const qsizetype newline = m_buffer.indexOf('\n');
            if (newline < 0)
                return m_buffer.size() <= kMaxHeaderBytes;
            if (newline > kMaxHeaderBytes)
                return false;
            QList<QByteArray> fields = m_buffer.left(newline).split(' ');
            fields.removeAll(QByteArray());
            m_buffer.remove(0, newline + 1);
            if (fields.size() < 2)
                return false;
            bool ok = false;
            const qint64 size = fields.takeLast().toLongLong(&ok);
            if (!ok || size < 0 || size > kMaxPayloadBytes)
                return false;
            m_pending = EngineFrame{fields, {}};
            m_payloadSize = size;
        }
        if (m_buffer.size() < m_payloadSize)
            return true;
        m_pending.payload = m_buffer.left(m_payloadSize);
        m_buffer.remove(0, m_payloadSize);
        frames.append(std::move(m_pending));
        m_pending = {};
        m_payloadSize = -1;
    }
}

void FrameParser::reset()
{
    m_buffer.clear();
    m_pending = {};
    m_payloadSize = -1;
}

QByteArray encodeFrame(const QList<QByteArray>& fields, const QByteArray& payload)
{
    QByteArray frame;
    frame.reserve(payload.size() + 64);
    for (const QByteArray& field : fields) {
        frame.append(field);
        frame.append(' ');
    }
    frame.append(QByteArray::number(payload.size()));
    frame.append('\n');
    frame.append(payload);
    return frame;
}
