#pragma once

#include "FrameParser.h"

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

// Owns one helper process. The model stays loaded between requests and is unloaded when idle.
class EngineProcess : public QObject
{
    Q_OBJECT

public:
    explicit EngineProcess(QObject* parent = nullptr);
    ~EngineProcess() override;

    bool ensureRunning(const QString& program, const QStringList& arguments, QString* error);
    void send(const QList<QByteArray>& fields, const QByteArray& payload = {});
    bool isRunning() const;
    bool usesFile(const QString& path) const;
    void stop();

signals:
    void frameReceived(const EngineFrame& frame);
    void failed();

private:
    void onReadyRead();
    void onFinished();

    QProcess* m_process = nullptr;
    QString m_program;
    QStringList m_arguments;
    FrameParser m_parser;
    QTimer m_idleTimer;
    bool m_stopping = false;
};
