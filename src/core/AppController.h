#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

class AppSettings;
class AudioPlayer;
class AudioRecorder;
class EngineProcess;
class ModelManager;
struct EngineFrame;

// Runs one heavy task at a time (record, transcribe, translate or synthesize) and ignores stale results.
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString translatedText READ translatedText NOTIFY translatedTextChanged)
    Q_PROPERTY(bool translationComplete READ translationComplete NOTIFY translatedTextChanged)
    Q_PROPERTY(QString detectedLanguage READ detectedLanguage NOTIFY detectedLanguageChanged)
    Q_PROPERTY(double recordingSeconds READ recordingSeconds NOTIFY recordingSecondsChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(QVariantList targetLanguages READ targetLanguages NOTIFY languagesChanged)
    Q_PROPERTY(QVariantList sourceLanguages READ sourceLanguages NOTIFY languagesChanged)

public:
    AppController(AppSettings* settings, ModelManager* models, QObject* parent = nullptr);
    ~AppController() override;

    QString state() const { return m_state; }
    bool busy() const { return m_state != QLatin1String("idle"); }
    QString statusText() const { return m_statusText; }
    QString translatedText() const { return m_translatedText; }
    bool translationComplete() const { return m_translationComplete; }
    QString detectedLanguage() const { return m_detectedLanguage; }
    double recordingSeconds() const { return m_recordingSeconds; }
    bool playing() const;
    QVariantList targetLanguages() const;
    QVariantList sourceLanguages() const;

    Q_INVOKABLE void translate(const QString& text, const QString& source, const QString& target);
    Q_INVOKABLE void startRecording(const QString& source);
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void speak(const QString& text, const QString& language);
    Q_INVOKABLE void stopAudio();
    Q_INVOKABLE bool canSpeak(const QString& language) const;
    Q_INVOKABLE QString languageName(const QString& code) const;
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE void copyToClipboard(const QString& text) const;
    // Bundled legal document by key: "privacy", "license" or "notices".
    Q_INVOKABLE QString document(const QString& name) const;
    Q_INVOKABLE void setVoiceInput(bool voiceInput) { m_voiceInput = voiceInput; }
    Q_INVOKABLE void clearTranslation();
    // Length checks use UTF-16 code units, the same unit as QML's text.length.
    Q_INVOKABLE bool fitsLimit(int currentLength, int selectedLength, int insertedLength, int maxLength) const;

signals:
    void stateChanged();
    void statusTextChanged();
    void translatedTextChanged();
    void detectedLanguageChanged();
    void recordingSecondsChanged();
    void playingChanged();
    void languagesChanged();
    void transcriptReady(const QString& text, const QString& language);
    void notice(const QString& message);

private:
    void setState(const QString& state);
    void setStatus(const QString& text);
    void beginRecording(const QString& source);
    void onRecordingFinished(bool limitReached);
    void onTranslateFrame(const EngineFrame& frame);
    void onTranscribeFrame(const EngineFrame& frame);
    void onSpeakFrame(const EngineFrame& frame);
    void onEngineFailed(const QString& task);
    void onWatchdog();
    void onModelRemoval(const QString& id);
    void finishTask(const QString& status = {});
    bool startEngine(EngineProcess* engine, const QString& executable, const QStringList& arguments);
    QString engineErrorMessage(const QByteArray& code) const;
    QString threadCount() const;

    AppSettings* m_settings;
    ModelManager* m_models;
    AudioRecorder* m_recorder;
    AudioPlayer* m_player;
    EngineProcess* m_translateEngine;
    EngineProcess* m_transcribeEngine;
    EngineProcess* m_speakEngine;
    QTimer m_watchdog;

    QString m_state = QStringLiteral("idle");
    QString m_statusText;
    QString m_translatedText;
    bool m_translationComplete = true;
    QString m_detectedLanguage;
    QString m_recordSource;
    QString m_translationTarget;
    double m_recordingSeconds = 0.0;
    bool m_voiceInput = false;
    bool m_speakAfterTranslation = false;
    quint64 m_nextRequest = 0;
    QByteArray m_activeRequest;
    QByteArray m_speakRequest;
};
