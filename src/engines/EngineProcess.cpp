#include "EngineProcess.h"

#include <QFileInfo>

namespace {
constexpr int kIdleUnloadMs = 10 * 60 * 1000;
}

EngineProcess::EngineProcess(QObject* parent) : QObject(parent)
{
    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(kIdleUnloadMs);
    connect(&m_idleTimer, &QTimer::timeout, this, &EngineProcess::stop);
}

EngineProcess::~EngineProcess()
{
    stop();
}

bool EngineProcess::ensureRunning(const QString& program, const QStringList& arguments, QString* error)
{
    if (isRunning() && program == m_program && arguments == m_arguments) {
        m_idleTimer.start();
        return true;
    }
    stop();

    if (!QFileInfo(program).isExecutable()) {
        if (error)
            *error = program;
        return false;
    }

    m_program = program;
    m_arguments = arguments;
    m_parser.reset();
    m_process = new QProcess(this);
    m_process->setProgram(program);
    m_process->setArguments(arguments);
    // Helper diagnostics may echo user content, so they are discarded unless explicitly debugging.
    if (qEnvironmentVariableIsSet("DUCKTOR_DEBUG_ENGINES"))
        m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    else
        m_process->setStandardErrorFile(QProcess::nullDevice());
    connect(m_process, &QProcess::readyReadStandardOutput, this, &EngineProcess::onReadyRead);
    connect(m_process, &QProcess::finished, this, &EngineProcess::onFinished);
    m_process->start();
    if (!m_process->waitForStarted(10000)) {
        if (error)
            *error = m_process->errorString();
        m_process->deleteLater();
        m_process = nullptr;
        return false;
    }
    m_idleTimer.start();
    return true;
}

void EngineProcess::send(const QList<QByteArray>& fields, const QByteArray& payload)
{
    if (!isRunning())
        return;
    m_process->write(encodeFrame(fields, payload));
    m_idleTimer.start();
}

bool EngineProcess::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool EngineProcess::usesFile(const QString& path) const
{
    return isRunning() && m_arguments.contains(path);
}

void EngineProcess::stop()
{
    m_idleTimer.stop();
    if (!m_process)
        return;
    m_stopping = true;
    QProcess* process = m_process;
    m_process = nullptr;
    process->disconnect(this);
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(3000);
    }
    process->deleteLater();
    m_stopping = false;
}

void EngineProcess::onReadyRead()
{
    if (!m_process)
        return;
    QList<EngineFrame> frames;
    if (!m_parser.feed(m_process->readAllStandardOutput(), frames)) {
        stop();
        emit failed();
        return;
    }
    m_idleTimer.start();
    for (const EngineFrame& frame : std::as_const(frames))
        emit frameReceived(frame);
}

void EngineProcess::onFinished()
{
    if (m_stopping || !m_process)
        return;
    onReadyRead();
    if (!m_process)
        return;
    m_process->deleteLater();
    m_process = nullptr;
    m_idleTimer.stop();
    emit failed();
}
