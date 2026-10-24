#pragma once

#include "ModelCatalog.h"

#include <QAbstractItemModel>
#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QVariantList>

#include <memory>

class AppSettings;
class ModelListModel;
class VoiceTableModel;
class QCryptographicHash;
class QNetworkAccessManager;
class QNetworkReply;

// Catalog state, verified downloads, offline import and the active model per capability.
class ModelManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* translationModels READ translationModels CONSTANT)
    Q_PROPERTY(QAbstractItemModel* speechToTextModels READ speechToTextModels CONSTANT)
    Q_PROPERTY(QAbstractItemModel* textToSpeechModels READ textToSpeechModels CONSTANT)
    Q_PROPERTY(QAbstractItemModel* voiceTable READ voiceTable CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString modelsDirectory READ modelsDirectory CONSTANT)

public:
    ModelManager(AppSettings* settings, const QString& userModelsDir, const QStringList& bundledDirs,
                 QObject* parent = nullptr);
    ~ModelManager() override;

    void loadCatalog(const QByteArray& json);
    // Refreshes texts that depend on the interface language.
    void relocalize();

    QAbstractItemModel* translationModels() const;
    QAbstractItemModel* speechToTextModels() const;
    QAbstractItemModel* textToSpeechModels() const;
    QAbstractItemModel* voiceTable() const;
    int revision() const { return m_revision; }
    bool downloading() const { return m_download != nullptr; }
    bool importing() const { return m_importing; }
    QString modelsDirectory() const { return m_userDir; }

    const QList<ModelInfo>& models() const { return m_models; }
    const ModelInfo* model(const QString& id) const;
    bool isInstalled(const QString& id) const;
    bool isBundled(const QString& id) const;
    QString filePath(const QString& id, const QString& fileName) const;
    QString status(const QString& id) const;
    double progress(const QString& id) const;
    bool isActive(const QString& id) const;
    QString activeModelId(const QString& capability) const;
    QString voiceFor(const QString& language) const;
    QString voiceGender(const QString& language) const;
    int voiceSpeaker(const QString& language) const;
    bool isVoiceActive(const QString& language, const QString& id, const QString& gender) const;

    Q_INVOKABLE QVariantList installedModels(const QString& capability, const QString& language = {}) const;
    Q_INVOKABLE QString selectedModel(const QString& capability, const QString& language = {}) const;
    Q_INVOKABLE void selectModel(const QString& capability, const QString& id, const QString& language = {});
    Q_INVOKABLE void selectVoice(const QString& language, const QString& id, const QString& gender);
    Q_INVOKABLE QString languageName(const QString& code) const;
    Q_INVOKABLE void download(const QString& id);
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE void importFile(const QUrl& url);

signals:
    void revisionChanged();
    void downloadingChanged();
    void importingChanged();
    void activeModelChanged(const QString& capability);
    void aboutToRemove(const QString& id);
    void message(const QString& text);

private:
    struct Download;
    struct ImportResult;

    QString userFilePath(const QString& id, const QString& fileName) const;
    QString bundledFilePath(const QString& id, const QString& fileName) const;
    bool hasFile(const QString& path, qint64 size) const;
    QString settingKey(const QString& capability, const QString& language) const;
    void startNextFile();
    void resumeFile(quint64 token, std::shared_ptr<QCryptographicHash> hash, qint64 offset);
    void onReadyRead();
    void onFinished();
    void abortDownload(const QString& error);
    void finishDownload(bool success, const QString& error);
    void finishImport(const ImportResult& result);
    void changed(const QString& capability);
    void reloadLists();
    void refreshLists(const QString& id);

    AppSettings* m_settings;
    QString m_userDir;
    QStringList m_bundledDirs;
    QList<ModelInfo> m_models;
    ModelListModel* m_translationList = nullptr;
    ModelListModel* m_speechToTextList = nullptr;
    ModelListModel* m_textToSpeechList = nullptr;
    VoiceTableModel* m_voiceTable = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    std::unique_ptr<Download> m_download;
    quint64 m_nextToken = 0;
    bool m_importing = false;
    int m_revision = 0;
};
