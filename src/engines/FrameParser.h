#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

struct EngineFrame
{
    QList<QByteArray> fields;
    QByteArray payload;

    QByteArray command() const { return fields.value(0); }
    QByteArray field(int index) const { return fields.value(index); }
};

// Incremental parser for the helper protocol: "FIELD ... <payloadBytes>\n<payload>".
class FrameParser
{
public:
    static constexpr int kMaxHeaderBytes = 256;
    static constexpr qint64 kMaxPayloadBytes = 32 * 1024 * 1024;

    // Returns false on malformed or oversized input; the stream cannot be resumed afterwards.
    bool feed(const QByteArray& data, QList<EngineFrame>& frames);
    void reset();

private:
    QByteArray m_buffer;
    EngineFrame m_pending;
    qint64 m_payloadSize = -1;
};

QByteArray encodeFrame(const QList<QByteArray>& fields, const QByteArray& payload);
