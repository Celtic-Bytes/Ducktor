#include "AppController.h"

#include "AppSettings.h"
#include "Languages.h"
#include "Paths.h"
#include "audio/AudioDsp.h"
#include "audio/AudioPlayer.h"
#include "audio/AudioRecorder.h"
#include "engines/EngineProcess.h"
#include "models/ModelManager.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QThread>
#include <QtEndian>

#if QT_CONFIG(permissions)
#include <QPermissions>
#endif

#include <algorithm>

static_assert(Q_BYTE_ORDER == Q_LITTLE_ENDIAN, "The engine protocol sends raw little-endian samples");

namespace {
constexpr int kTranslateTimeoutMs = 20 * 60 * 1000;
constexpr int kTranscribeTimeoutMs = 5 * 60 * 1000;
constexpr int kSpeakTimeoutMs = 5 * 60 * 1000;
constexpr int kMaxSpeakChars = 20000;
constexpr int kMaxSpeechSeconds = 600;

const QString kIdle = QStringLiteral("idle");
const QString kRecording = QStringLiteral("recording");
const QString kTranscribing = QStringLiteral("transcribing");
const QString kTranslating = QStringLiteral("translating");
const QString kSynthesizing = QStringLiteral("synthesizing");
const QString kAuto = QStringLiteral("auto");
} // namespace

AppController::AppController(AppSettings* settings, ModelManager* models, QObject* parent)
    : QObject(parent), m_settings(settings), m_models(models), m_recorder(new AudioRecorder(this)),
      m_player(new AudioPlayer(this)), m_translateEngine(new EngineProcess(this)),
      m_transcribeEngine(new EngineProcess(this)), m_speakEngine(new EngineProcess(this))
{
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, &AppController::onWatchdog);

    connect(m_recorder, &AudioRecorder::progress, this, [this](double seconds) {
        m_recordingSeconds = seconds;
        emit recordingSecondsChanged();
    });
    connect(m_recorder, &AudioRecorder::finished, this, &AppController::onRecordingFinished);
    connect(m_player, &AudioPlayer::activeChanged, this, &AppController::playingChanged);

    connect(m_translateEngine, &EngineProcess::frameReceived, this, &AppController::onTranslateFrame);
    connect(m_transcribeEngine, &EngineProcess::frameReceived, this, &AppController::onTranscribeFrame);
    connect(m_speakEngine, &EngineProcess::frameReceived, this, &AppController::onSpeakFrame);
    connect(m_translateEngine, &EngineProcess::failed, this, [this]() { onEngineFailed(kTranslating); });
    connect(m_transcribeEngine, &EngineProcess::failed, this, [this]() { onEngineFailed(kTranscribing); });
    connect(m_speakEngine, &EngineProcess::failed, this, [this]() { onEngineFailed(kSynthesizing); });

    connect(m_models, &ModelManager::activeModelChanged, this, [this](const QString& capability) {
        if (capability == Capability::Translation)
            emit languagesChanged();
    });
    connect(m_models, &ModelManager::aboutToRemove, this, &AppController::onModelRemoval);
}

AppController::~AppController()
{
    m_recorder->cancel();
    m_player->stop();
}

bool AppController::playing() const
{
    return m_player->isActive();
}

QVariantList AppController::targetLanguages() const
{
    const ModelInfo* info = m_models->model(m_models->activeModelId(Capability::Translation));
    return Languages::options(info ? info->languages : QStringList{QStringLiteral("es"), QStringLiteral("en")});
}

QVariantList AppController::sourceLanguages() const
{
    QVariantList languages{QVariantMap{{QStringLiteral("code"), kAuto}, {QStringLiteral("name"), tr("Detect language")}}};
    languages << targetLanguages();
    return languages;
}

bool AppController::fitsLimit(int currentLength, int selectedLength, int insertedLength, int maxLength) const
{
    return qint64(currentLength) - qint64(selectedLength) + qint64(insertedLength) <= qint64(maxLength);
}

QString AppController::languageName(const QString& code) const
{
    return code == kAuto ? tr("Detect language") : Languages::displayName(code);
}

QString AppController::clipboardText() const
{
    return QGuiApplication::clipboard()->text();
}

void AppController::copyToClipboard(const QString& text) const
{
    QGuiApplication::clipboard()->setText(text);
}

QString AppController::document(const QString& name) const
{
    static const QHash<QString, QString> files{{QStringLiteral("privacy"), QStringLiteral("PRIVACY.md")},
                                              {QStringLiteral("license"), QStringLiteral("LICENSE")},
                                              {QStringLiteral("notices"), QStringLiteral("THIRD_PARTY_NOTICES.md")}};
    const QString file = files.value(name);
    if (file.isEmpty())
        return {};
    QFile resource(QStringLiteral(":/docs/") + file);
    return resource.open(QIODevice::ReadOnly) ? QString::fromUtf8(resource.readAll()) : QString();
}

void AppController::clearTranslation()
{
    m_translatedText.clear();
    m_translationComplete = true;
    emit translatedTextChanged();
}

bool AppController::canSpeak(const QString& language) const
{
    return !m_models->voiceFor(language).isEmpty();
}

void AppController::translate(const QString& text, const QString& source, const QString& target)
{
    if (busy())
        return;
    if (text.trimmed().isEmpty()) {
        emit notice(tr("Write or paste some text to translate."));
        return;
    }
    const int limit = m_settings->maxInputChars();
    if (text.size() > limit) {
        emit notice(tr("The text has %1 characters and the limit is %2. Shorten it before translating.")
                        .arg(text.size())
                        .arg(limit));
        return;
    }
    if (source == target) {
        emit notice(tr("The source and target languages are the same."));
        return;
    }

    const QString modelId = m_models->activeModelId(Capability::Translation);
    const ModelInfo* info = m_models->model(modelId);
    if (!info) {
        emit notice(tr("No translation model is installed. Download one in Settings."));
        return;
    }
    if (!info->languages.contains(target)) {
        emit notice(tr("The active translation model does not support %1.").arg(languageName(target)));
        return;
    }

    const QString promptLanguage = info->options.value(QStringLiteral("promptLanguage"), QStringLiteral("en")).toString();
    const QStringList arguments{
        QStringLiteral("--model"), m_models->filePath(modelId, info->files.first().name),
        QStringLiteral("--ctx"), info->options.value(QStringLiteral("contextSize"), 4096).toString(),
        QStringLiteral("--threads"), threadCount(),
        QStringLiteral("--prompt-language"), promptLanguage,
        QStringLiteral("--sampling"), info->options.value(QStringLiteral("sampling"), QStringLiteral("greedy")).toString(),
        QStringLiteral("--no-think"), info->options.value(QStringLiteral("noThink")).toBool() ? QStringLiteral("1") : QStringLiteral("0")};
    if (!startEngine(m_translateEngine, QStringLiteral("ducktor-translate"), arguments))
        return;

    const QString sourceName = source == kAuto ? QString() : Languages::promptName(source, promptLanguage);
    QByteArray payload = sourceName.toUtf8();
    payload += '\x1f';
    payload += Languages::promptName(target, promptLanguage).toUtf8();
    payload += '\x1f';
    payload += text.toUtf8();

    m_player->stop();
    m_activeRequest = QByteArray::number(++m_nextRequest);
    m_translationTarget = target;
    m_speakAfterTranslation = m_voiceInput && m_settings->autoPlayVoiceResults();
    clearTranslation();
    m_translateEngine->send({QByteArrayLiteral("TRANSLATE"), m_activeRequest}, payload);
    setState(kTranslating);
    setStatus(tr("Translating…"));
    m_watchdog.start(kTranslateTimeoutMs);
}

void AppController::startRecording(const QString& source)
{
    if (busy())
        return;
    if (m_models->activeModelId(Capability::SpeechToText).isEmpty()) {
        emit notice(tr("No speech recognition model is installed. Download one in Settings."));
        return;
    }
#if QT_CONFIG(permissions)
    const QMicrophonePermission permission;
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, [this, source](const QPermission& result) {
            if (result.status() == Qt::PermissionStatus::Granted)
                beginRecording(source);
            else
                emit notice(tr("Microphone access was denied. Allow it in the system settings to use voice input."));
        });
        return;
    case Qt::PermissionStatus::Denied:
        emit notice(tr("Microphone access was denied. Allow it in the system settings to use voice input."));
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    beginRecording(source);
}

void AppController::beginRecording(const QString& source)
{
    if (busy())
        return;
    m_player->stop();
    QString error;
    if (!m_recorder->start(m_settings->maxRecordingSeconds(), &error)) {
        emit notice(error == QLatin1String("no_device") ? tr("No microphone was found.")
                                                         : tr("The microphone could not be opened."));
        return;
    }
    m_recordSource = source;
    m_recordingSeconds = 0.0;
    emit recordingSecondsChanged();
    setState(kRecording);
    setStatus(tr("Recording… Press stop when you finish speaking."));
}

void AppController::stopRecording()
{
    if (m_state == kRecording)
        m_recorder->stop();
}

void AppController::onRecordingFinished(bool limitReached)
{
    std::vector<float> samples = m_recorder->takeSamples();
    if (limitReached)
        emit notice(tr("Recording stopped at the %1-second limit.").arg(m_settings->maxRecordingSeconds()));
    if (samples.size() < std::size_t(AudioDsp::kSpeechSampleRate / 2)) {
        finishTask(tr("The recording is too short."));
        return;
    }
    if (AudioDsp::isProbablySilence(samples)) {
        finishTask(tr("No speech was detected."));
        return;
    }

    const QString modelId = m_models->activeModelId(Capability::SpeechToText);
    const ModelInfo* info = m_models->model(modelId);
    if (!info) {
        finishTask(tr("No speech recognition model is installed. Download one in Settings."));
        return;
    }
    const QStringList arguments{QStringLiteral("--model"), m_models->filePath(modelId, info->files.first().name),
                                QStringLiteral("--threads"), threadCount(), QStringLiteral("--max-seconds"),
                                QString::number(AppSettings::kHardMaxRecordingSeconds + 10)};
    if (!startEngine(m_transcribeEngine, QStringLiteral("ducktor-transcribe"), arguments)) {
        finishTask();
        return;
    }

    const QByteArray payload(reinterpret_cast<const char*>(samples.data()), qsizetype(samples.size() * sizeof(float)));
    m_activeRequest = QByteArray::number(++m_nextRequest);
    const QString language = m_recordSource.isEmpty() ? kAuto : m_recordSource;
    m_transcribeEngine->send({QByteArrayLiteral("TRANSCRIBE"), m_activeRequest, language.toLatin1()}, payload);
    setState(kTranscribing);
    setStatus(tr("Transcribing…"));
    m_watchdog.start(kTranscribeTimeoutMs);
}

void AppController::speak(const QString& text, const QString& language)
{
    if (busy() || text.trimmed().isEmpty())
        return;
    if (text.size() > kMaxSpeakChars) {
        emit notice(tr("The text is too long to read aloud."));
        return;
    }
    const ModelInfo* info = m_models->model(m_models->voiceFor(language));
    if (!info) {
        emit notice(tr("No voice is installed for %1. You can download one in Settings.").arg(languageName(language)));
        return;
    }
    const QString espeakData = Paths::espeakDataDir();
    if (espeakData.isEmpty()) {
        emit notice(tr("The speech synthesis data is missing from this installation."));
        return;
    }
    QString modelPath;
    QString configPath;
    for (const ModelFile& file : info->files) {
        if (file.name.endsWith(QLatin1String(".onnx.json")))
            configPath = m_models->filePath(info->id, file.name);
        else if (file.name.endsWith(QLatin1String(".onnx")))
            modelPath = m_models->filePath(info->id, file.name);
    }
    const QStringList arguments{QStringLiteral("--model"), modelPath, QStringLiteral("--config"), configPath,
                                QStringLiteral("--espeak-data"), espeakData, QStringLiteral("--max-seconds"),
                                QString::number(kMaxSpeechSeconds)};
    if (!startEngine(m_speakEngine, QStringLiteral("ducktor-speak"), arguments))
        return;

    m_player->stop();
    m_speakRequest = QByteArray::number(++m_nextRequest);
    m_speakEngine->send({QByteArrayLiteral("SPEAK"), m_speakRequest}, text.toUtf8());
    setState(kSynthesizing);
    setStatus(tr("Generating speech…"));
    m_watchdog.start(kSpeakTimeoutMs);
}

void AppController::stopAudio()
{
    m_player->stop();
    if (m_state == kSynthesizing) {
        m_speakEngine->stop();
        m_speakRequest.clear();
        finishTask();
    }
}

void AppController::cancel()
{
    if (m_state == kRecording) {
        m_recorder->cancel();
    } else if (m_state == kTranscribing) {
        m_transcribeEngine->stop();
    } else if (m_state == kTranslating) {
        m_translateEngine->stop();
    } else if (m_state == kSynthesizing) {
        stopAudio();
        return;
    } else {
        return;
    }
    finishTask(tr("Cancelled."));
}

void AppController::onTranslateFrame(const EngineFrame& frame)
{
    const QByteArray command = frame.command();
    if (m_state != kTranslating)
        return;
    if (command == "ERROR" && frame.field(1) == "0") {
        m_translateEngine->stop();
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
        return;
    }
    if (frame.field(1) != m_activeRequest)
        return;

    if (command == "PROGRESS") {
        setStatus(tr("Translating… (%1/%2)").arg(QString::fromLatin1(frame.field(2)), QString::fromLatin1(frame.field(3))));
    } else if (command == "RESULT") {
        m_translatedText = QString::fromUtf8(frame.payload);
        m_translationComplete = frame.field(2) == "1";
        emit translatedTextChanged();
        finishTask(m_translationComplete ? QString()
                                         : tr("The translation may be incomplete: the model stopped before finishing."));
        if (m_speakAfterTranslation && !m_translatedText.isEmpty())
            speak(m_translatedText, m_translationTarget);
    } else if (command == "ERROR") {
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
    }
}

void AppController::onTranscribeFrame(const EngineFrame& frame)
{
    const QByteArray command = frame.command();
    if (m_state != kTranscribing)
        return;
    if (command == "ERROR" && frame.field(1) == "0") {
        m_transcribeEngine->stop();
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
        return;
    }
    if (frame.field(1) != m_activeRequest)
        return;

    if (command == "TRANSCRIPT") {
        const QString text = QString::fromUtf8(frame.payload);
        const QString language = QString::fromLatin1(frame.field(2));
        if (m_recordSource == kAuto) {
            m_detectedLanguage = language;
            emit detectedLanguageChanged();
        }
        m_voiceInput = true;
        finishTask();
        emit transcriptReady(text, language);
        if (text.size() > m_settings->maxInputChars()) {
            emit notice(tr("The transcription exceeds the %1-character limit. Shorten it before translating.")
                            .arg(m_settings->maxInputChars()));
        }
    } else if (command == "ERROR") {
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
    }
}

void AppController::onSpeakFrame(const EngineFrame& frame)
{
    const QByteArray command = frame.command();
    if (m_state != kSynthesizing)
        return;
    if (command == "ERROR" && frame.field(1) == "0") {
        m_speakEngine->stop();
        m_speakRequest.clear();
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
        return;
    }
    if (m_speakRequest.isEmpty() || frame.field(1) != m_speakRequest)
        return;

    if (command == "AUDIO") {
        if (!m_player->isActive() && !m_player->begin(frame.field(2).toInt())) {
            m_speakEngine->stop();
            m_speakRequest.clear();
            finishTask();
            emit notice(tr("The audio output could not be opened."));
            return;
        }
        m_player->append(frame.payload);
    } else if (command == "DONE") {
        m_player->endOfStream();
        m_speakRequest.clear();
        finishTask(frame.field(2) == "1" ? QString() : tr("The audio was cut at the maximum length."));
    } else if (command == "ERROR") {
        m_player->stop();
        m_speakRequest.clear();
        finishTask();
        emit notice(engineErrorMessage(frame.field(2)));
    }
}

void AppController::onEngineFailed(const QString& task)
{
    if (task == kSynthesizing)
        m_player->stop();
    if (m_state != task)
        return;
    m_speakRequest.clear();
    finishTask();
    emit notice(tr("The engine stopped unexpectedly. The model may need more memory than is available."));
}

void AppController::onWatchdog()
{
    if (m_state == kTranslating)
        m_translateEngine->stop();
    else if (m_state == kTranscribing)
        m_transcribeEngine->stop();
    else if (m_state == kSynthesizing)
        stopAudio();
    else
        return;
    finishTask(tr("The operation took too long and was stopped."));
}

void AppController::onModelRemoval(const QString& id)
{
    const ModelInfo* info = m_models->model(id);
    if (!info)
        return;
    for (const ModelFile& file : info->files) {
        const QString path = m_models->filePath(id, file.name);
        for (EngineProcess* engine : {m_translateEngine, m_transcribeEngine, m_speakEngine}) {
            if (!engine->usesFile(path))
                continue;
            if ((engine == m_translateEngine && m_state == kTranslating) ||
                (engine == m_transcribeEngine && m_state == kTranscribing) ||
                (engine == m_speakEngine && m_state == kSynthesizing))
                cancel();
            engine->stop();
        }
    }
}

void AppController::finishTask(const QString& status)
{
    m_watchdog.stop();
    m_activeRequest.clear();
    setState(kIdle);
    setStatus(status);
}

bool AppController::startEngine(EngineProcess* engine, const QString& executable, const QStringList& arguments)
{
    const QString program = Paths::engineExecutable(executable);
    if (program.isEmpty()) {
        emit notice(tr("A required component (%1) is missing from this installation.").arg(executable));
        return false;
    }
    QString error;
    if (!engine->ensureRunning(program, arguments, &error)) {
        emit notice(tr("The %1 component could not be started.").arg(executable));
        return false;
    }
    return true;
}

QString AppController::engineErrorMessage(const QByteArray& code) const
{
    if (code == "load_failed")
        return tr("The model could not be loaded. The file may be damaged, incompatible, or too large for the available memory.");
    if (code == "too_long")
        return tr("The input is too long for this model.");
    if (code == "too_short")
        return tr("The recording is too short.");
    if (code == "no_speech")
        return tr("No speech was detected.");
    if (code == "unsupported_template")
        return tr("This model uses a prompt format that is not supported yet.");
    if (code == "decode_failed")
        return tr("The model failed while processing the input.");
    if (code == "synth_failed")
        return tr("Speech synthesis failed.");
    return tr("Unexpected engine error (%1).").arg(QString::fromLatin1(code));
}

QString AppController::threadCount() const
{
    return QString::number(std::clamp(QThread::idealThreadCount() - 1, 1, 8));
}

void AppController::setState(const QString& state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

void AppController::setStatus(const QString& text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}
