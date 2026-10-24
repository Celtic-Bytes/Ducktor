#include "ModelManager.h"

#include "ModelListModel.h"
#include "VoiceTableModel.h"
#include "core/AppSettings.h"
#include "core/Languages.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStorageInfo>
#include <QThreadPool>

namespace {
constexpr qint64 kDiskMarginBytes = 200LL * 1024 * 1024;
constexpr qint64 kProgressIntervalMs = 250;
} // namespace

struct ModelManager::Download
{
    QString modelId;
    int fileIndex = 0;
    QFile file;
    std::shared_ptr<QCryptographicHash> hash;
    QPointer<QNetworkReply> reply;
    qint64 offset = 0;
    qint64 completedBytes = 0;
    quint64 token = 0;
    bool statusChecked = false;
    bool cancelled = false;
    bool verifyingPart = false;
    QString error;
    QElapsedTimer progressTimer;
};

struct ModelManager::ImportResult
{
    QString modelId;
    QString fileName;
    QString error;
};

ModelManager::ModelManager(AppSettings* settings, const QString& userModelsDir, const QStringList& bundledDirs,
                           QObject* parent)
    : QObject(parent), m_settings(settings), m_userDir(userModelsDir), m_bundledDirs(bundledDirs),
      m_network(new QNetworkAccessManager(this))
{
    m_network->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_translationList = new ModelListModel(this, Capability::Translation, this);
    m_speechToTextList = new ModelListModel(this, Capability::SpeechToText, this);
    m_textToSpeechList = new ModelListModel(this, Capability::TextToSpeech, this);
    m_voiceTable = new VoiceTableModel(this, this);
}

ModelManager::~ModelManager()
{
    if (m_download && m_download->reply) {
        m_download->cancelled = true;
        m_download->reply->disconnect(this);
        m_download->reply->abort();
    }
}

void ModelManager::loadCatalog(const QByteArray& json)
{
    QStringList warnings;
    m_models = ModelCatalog::parse(json, &warnings);
    for (const QString& warning : std::as_const(warnings))
        qWarning("%s", qPrintable(warning));
    reloadLists();
    ++m_revision;
    emit revisionChanged();
}

void ModelManager::relocalize()
{
    reloadLists();
    ++m_revision;
    emit revisionChanged();
}

QAbstractItemModel* ModelManager::translationModels() const { return m_translationList; }
QAbstractItemModel* ModelManager::speechToTextModels() const { return m_speechToTextList; }
QAbstractItemModel* ModelManager::textToSpeechModels() const { return m_textToSpeechList; }
QAbstractItemModel* ModelManager::voiceTable() const { return m_voiceTable; }

const ModelInfo* ModelManager::model(const QString& id) const
{
    for (const ModelInfo& info : m_models) {
        if (info.id == id)
            return &info;
    }
    return nullptr;
}

QString ModelManager::userFilePath(const QString& id, const QString& fileName) const
{
    return QDir(m_userDir).filePath(id + QLatin1Char('/') + fileName);
}

QString ModelManager::bundledFilePath(const QString& id, const QString& fileName) const
{
    for (const QString& dir : m_bundledDirs) {
        const QString path = QDir(dir).filePath(id + QLatin1Char('/') + fileName);
        if (QFileInfo::exists(path))
            return path;
    }
    return {};
}

bool ModelManager::hasFile(const QString& path, qint64 size) const
{
    const QFileInfo info(path);
    return !path.isEmpty() && info.isFile() && info.size() == size;
}

bool ModelManager::isBundled(const QString& id) const
{
    const ModelInfo* info = model(id);
    if (!info || !info->available)
        return false;
    for (const ModelFile& file : info->files) {
        if (!hasFile(bundledFilePath(id, file.name), file.size))
            return false;
    }
    return true;
}

bool ModelManager::isInstalled(const QString& id) const
{
    const ModelInfo* info = model(id);
    if (!info || !info->available)
        return false;
    for (const ModelFile& file : info->files) {
        if (filePath(id, file.name).isEmpty())
            return false;
    }
    return true;
}

QString ModelManager::filePath(const QString& id, const QString& fileName) const
{
    const ModelInfo* info = model(id);
    if (!info)
        return {};
    for (const ModelFile& file : info->files) {
        if (file.name != fileName)
            continue;
        const QString user = userFilePath(id, file.name);
        if (hasFile(user, file.size))
            return user;
        const QString bundled = bundledFilePath(id, file.name);
        if (hasFile(bundled, file.size))
            return bundled;
    }
    return {};
}

QString ModelManager::status(const QString& id) const
{
    const ModelInfo* info = model(id);
    if (!info || !info->available)
        return QStringLiteral("unavailable");
    if (m_download && m_download->modelId == id)
        return m_download->verifyingPart ? QStringLiteral("verifying") : QStringLiteral("downloading");
    if (isBundled(id))
        return QStringLiteral("bundled");
    if (isInstalled(id))
        return QStringLiteral("installed");
    return QStringLiteral("notInstalled");
}

double ModelManager::progress(const QString& id) const
{
    if (!m_download || m_download->modelId != id)
        return 0.0;
    const ModelInfo* info = model(id);
    const qint64 total = info ? info->totalSize() : 0;
    return total > 0 ? double(m_download->completedBytes + m_download->offset) / double(total) : 0.0;
}

bool ModelManager::isActive(const QString& id) const
{
    const ModelInfo* info = model(id);
    if (!info)
        return false;
    if (info->capability == Capability::TextToSpeech)
        return voiceFor(info->voiceLanguage) == id;
    return activeModelId(info->capability) == id;
}

QString ModelManager::settingKey(const QString& capability, const QString& language) const
{
    if (capability == Capability::TextToSpeech)
        return QStringLiteral("models/voice/") + language;
    return QStringLiteral("models/active/") + capability;
}

QString ModelManager::activeModelId(const QString& capability) const
{
    const QString chosen = m_settings->value(settingKey(capability, {}));
    const ModelInfo* info = model(chosen);
    if (info && info->capability == capability && isInstalled(chosen))
        return chosen;
    for (const bool recommendedOnly : {true, false}) {
        for (const ModelInfo& candidate : m_models) {
            if (candidate.capability == capability && (!recommendedOnly || candidate.recommended) &&
                isInstalled(candidate.id))
                return candidate.id;
        }
    }
    return {};
}

QString ModelManager::voiceFor(const QString& language) const
{
    const QString chosen = m_settings->value(settingKey(Capability::TextToSpeech, language));
    const ModelInfo* info = model(chosen);
    if (info && info->voiceLanguage == language && isInstalled(chosen))
        return chosen;
    for (const bool recommendedOnly : {true, false}) {
        for (const ModelInfo& candidate : m_models) {
            if (candidate.capability == Capability::TextToSpeech && candidate.voiceLanguage == language &&
                (!recommendedOnly || candidate.recommended) && isInstalled(candidate.id))
                return candidate.id;
        }
    }
    return {};
}

QString ModelManager::voiceGender(const QString& language) const
{
    const ModelInfo* info = model(voiceFor(language));
    if (!info || info->voices.isEmpty())
        return {};
    const QString chosen = m_settings->value(QStringLiteral("models/voiceGender/") + language);
    if (m_settings->value(settingKey(Capability::TextToSpeech, language)) == info->id && info->voice(chosen))
        return chosen;
    return info->voices.first().gender;
}

int ModelManager::voiceSpeaker(const QString& language) const
{
    const ModelInfo* info = model(voiceFor(language));
    const VoiceOption* voice = info ? info->voice(voiceGender(language)) : nullptr;
    return voice ? voice->speaker : 0;
}

bool ModelManager::isVoiceActive(const QString& language, const QString& id, const QString& gender) const
{
    return !id.isEmpty() && voiceFor(language) == id && voiceGender(language) == gender;
}

QVariantList ModelManager::installedModels(const QString& capability, const QString& language) const
{
    QVariantList result;
    for (const ModelInfo& info : m_models) {
        if (info.capability != capability || !isInstalled(info.id))
            continue;
        if (!language.isEmpty() && info.voiceLanguage != language)
            continue;
        result << QVariantMap{{QStringLiteral("id"), info.id}, {QStringLiteral("name"), info.name}};
    }
    return result;
}

QString ModelManager::selectedModel(const QString& capability, const QString& language) const
{
    return capability == Capability::TextToSpeech ? voiceFor(language) : activeModelId(capability);
}

void ModelManager::selectModel(const QString& capability, const QString& id, const QString& language)
{
    const ModelInfo* info = model(id);
    if (!info || info->capability != capability || !isInstalled(id))
        return;
    if (capability == Capability::TextToSpeech && info->voiceLanguage != language)
        return;
    m_settings->setValue(settingKey(capability, language), id);
    changed(capability);
}

void ModelManager::selectVoice(const QString& language, const QString& id, const QString& gender)
{
    const ModelInfo* info = model(id);
    if (!info || info->capability != Capability::TextToSpeech || info->voiceLanguage != language ||
        !info->voice(gender) || !isInstalled(id))
        return;
    m_settings->setValue(settingKey(Capability::TextToSpeech, language), id);
    m_settings->setValue(QStringLiteral("models/voiceGender/") + language, gender);
    changed(Capability::TextToSpeech);
}

QString ModelManager::languageName(const QString& code) const
{
    return Languages::displayName(code);
}

void ModelManager::download(const QString& id)
{
    if (m_download || m_importing) {
        emit message(tr("Another download or import is in progress."));
        return;
    }
    const ModelInfo* info = model(id);
    if (!info || !info->available || isInstalled(id))
        return;

    const QString dir = QDir(m_userDir).filePath(id);
    if (!QDir().mkpath(dir)) {
        emit message(tr("The models folder could not be created."));
        return;
    }
    qint64 needed = 0;
    for (const ModelFile& file : info->files) {
        const QString path = userFilePath(id, file.name);
        if (!hasFile(path, file.size))
            needed += file.size - QFileInfo(path + QStringLiteral(".part")).size();
    }
    const QStorageInfo storage(dir);
    if (storage.isValid() && storage.bytesAvailable() < needed + kDiskMarginBytes) {
        emit message(tr("Not enough free disk space: %1 are needed.").arg(QLocale().formattedDataSize(needed)));
        return;
    }

    m_download = std::make_unique<Download>();
    m_download->modelId = id;
    m_download->token = ++m_nextToken;
    m_download->progressTimer.start();
    emit downloadingChanged();
    refreshLists(id);
    startNextFile();
}

void ModelManager::cancelDownload()
{
    if (!m_download)
        return;
    m_download->cancelled = true;
    if (m_download->reply)
        m_download->reply->abort();
    else
        finishDownload(false, {});
}

void ModelManager::startNextFile()
{
    Download& d = *m_download;
    const ModelInfo* info = model(d.modelId);
    while (d.fileIndex < info->files.size() &&
           hasFile(userFilePath(d.modelId, info->files[d.fileIndex].name), info->files[d.fileIndex].size)) {
        d.completedBytes += info->files[d.fileIndex].size;
        ++d.fileIndex;
    }
    if (d.fileIndex >= info->files.size()) {
        finishDownload(true, {});
        return;
    }

    const ModelFile& file = info->files[d.fileIndex];
    const QString partPath = userFilePath(d.modelId, file.name) + QStringLiteral(".part");
    const qint64 partSize = QFileInfo(partPath).size();
    if (partSize > 0 && partSize < file.size) {
        // Resume: re-hash the bytes already on disk without blocking the interface.
        d.verifyingPart = true;
        refreshLists(d.modelId);
        const quint64 token = d.token;
        QPointer<ModelManager> self(this);
        QThreadPool::globalInstance()->start([self, partPath, token]() {
            auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);
            QFile part(partPath);
            qint64 size = -1;
            if (part.open(QIODevice::ReadOnly) && hash->addData(&part))
                size = part.size();
            if (!self)
                return;
            QMetaObject::invokeMethod(
                self.data(), [self, hash, size, token]() {
                    if (self)
                        self->resumeFile(token, hash, size);
                },
                Qt::QueuedConnection);
        });
        return;
    }
    QFile::remove(partPath);
    resumeFile(d.token, std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256), 0);
}

void ModelManager::resumeFile(quint64 token, std::shared_ptr<QCryptographicHash> hash, qint64 offset)
{
    if (!m_download || m_download->token != token)
        return;
    Download& d = *m_download;
    if (d.cancelled) {
        finishDownload(false, {});
        return;
    }
    const ModelFile& file = model(d.modelId)->files[d.fileIndex];
    const QString partPath = userFilePath(d.modelId, file.name) + QStringLiteral(".part");
    if (offset < 0) {
        offset = 0;
        hash->reset();
    }

    d.verifyingPart = false;
    d.hash = std::move(hash);
    d.offset = offset;
    d.statusChecked = false;
    d.file.setFileName(partPath);
    if (!d.file.open(offset > 0 ? QIODevice::Append : (QIODevice::WriteOnly | QIODevice::Truncate))) {
        finishDownload(false, tr("The downloaded file could not be written."));
        return;
    }

    QNetworkRequest request(file.url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("Ducktor"));
    if (offset > 0)
        request.setRawHeader("Range", "bytes=" + QByteArray::number(offset) + '-');
    d.reply = m_network->get(request);
    connect(d.reply, &QNetworkReply::readyRead, this, &ModelManager::onReadyRead);
    connect(d.reply, &QNetworkReply::finished, this, &ModelManager::onFinished);
    refreshLists(d.modelId);
}

void ModelManager::onReadyRead()
{
    if (!m_download || !m_download->reply)
        return;
    Download& d = *m_download;
    const ModelFile& file = model(d.modelId)->files[d.fileIndex];

    if (!d.statusChecked) {
        d.statusChecked = true;
        const int status = d.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (d.offset > 0 && status == 200) {
            // The server ignored the range request: start the file again from zero.
            d.file.close();
            if (!d.file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                abortDownload(tr("The downloaded file could not be written."));
                return;
            }
            d.hash->reset();
            d.offset = 0;
        }
    }

    const QByteArray chunk = d.reply->readAll();
    if (d.offset + chunk.size() > file.size) {
        abortDownload(tr("The download is larger than expected and was stopped."));
        return;
    }
    if (d.file.write(chunk) != chunk.size()) {
        abortDownload(tr("The downloaded file could not be written."));
        return;
    }
    d.hash->addData(chunk);
    d.offset += chunk.size();
    if (d.progressTimer.elapsed() >= kProgressIntervalMs) {
        d.progressTimer.restart();
        refreshLists(d.modelId);
    }
}

void ModelManager::abortDownload(const QString& error)
{
    m_download->error = error;
    m_download->reply->abort();
}

void ModelManager::onFinished()
{
    if (!m_download || !m_download->reply)
        return;
    Download& d = *m_download;
    QNetworkReply* reply = d.reply;
    if (!d.cancelled && d.error.isEmpty() && reply->error() == QNetworkReply::NoError && reply->bytesAvailable() > 0)
        onReadyRead();
    if (!m_download)
        return;
    reply->deleteLater();
    d.reply = nullptr;
    d.file.close();

    const ModelFile& file = model(d.modelId)->files[d.fileIndex];
    const QString finalPath = userFilePath(d.modelId, file.name);
    const QString partPath = finalPath + QStringLiteral(".part");

    if (!d.error.isEmpty()) {
        QFile::remove(partPath);
        finishDownload(false, d.error);
        return;
    }
    if (d.cancelled) {
        finishDownload(false, {});
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        finishDownload(false, tr("The download failed: %1. You can retry to resume it.").arg(reply->errorString()));
        return;
    }
    if (d.offset != file.size || d.hash->result().toHex() != file.sha256) {
        QFile::remove(partPath);
        finishDownload(false, tr("The downloaded file failed verification and was deleted."));
        return;
    }
    QFile::remove(finalPath);
    if (!QFile::rename(partPath, finalPath)) {
        finishDownload(false, tr("The downloaded file could not be written."));
        return;
    }
    d.completedBytes += file.size;
    d.offset = 0;
    ++d.fileIndex;
    startNextFile();
}

void ModelManager::finishDownload(bool success, const QString& error)
{
    const QString id = m_download->modelId;
    m_download.reset();
    emit downloadingChanged();
    if (!error.isEmpty())
        emit message(error);
    const ModelInfo* info = model(id);
    if (success && info)
        emit message(tr("%1 is ready to use.").arg(info->name));
    changed(info ? info->capability : QString());
}

void ModelManager::remove(const QString& id)
{
    const ModelInfo* info = model(id);
    if (!info)
        return;
    if (m_download && m_download->modelId == id) {
        emit message(tr("Cancel the download before deleting this model."));
        return;
    }
    QDir dir(QDir(m_userDir).filePath(id));
    if (!dir.exists()) {
        if (isBundled(id))
            emit message(tr("Models included with Ducktor cannot be deleted."));
        return;
    }
    emit aboutToRemove(id);
    if (!dir.removeRecursively())
        emit message(tr("The model could not be deleted completely. Close any program using it and try again."));
    changed(info->capability);
}

void ModelManager::importFile(const QUrl& url)
{
    if (m_download || m_importing) {
        emit message(tr("Another download or import is in progress."));
        return;
    }
    const QString path = url.toLocalFile();
    const QFileInfo source(path);
    if (!source.isFile()) {
        emit message(tr("The selected file could not be read."));
        return;
    }

    struct Candidate
    {
        QString modelId;
        ModelFile file;
    };
    QList<Candidate> candidates;
    for (const ModelInfo& info : std::as_const(m_models)) {
        for (const ModelFile& file : info.files) {
            if (file.size == source.size())
                candidates << Candidate{info.id, file};
        }
    }
    if (candidates.isEmpty()) {
        emit message(tr("This file does not match any model in the catalog."));
        return;
    }

    m_importing = true;
    emit importingChanged();
    const QString userDir = m_userDir;
    QPointer<ModelManager> self(this);
    QThreadPool::globalInstance()->start([self, path, candidates, userDir]() {
        ImportResult result;
        QFile input(path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!input.open(QIODevice::ReadOnly) || !hash.addData(&input)) {
            result.error = QStringLiteral("read");
        } else {
            input.close();
            const QByteArray digest = hash.result().toHex();
            result.error = QStringLiteral("mismatch");
            for (const Candidate& candidate : candidates) {
                if (candidate.file.sha256 != digest)
                    continue;
                const QString dir = QDir(userDir).filePath(candidate.modelId);
                const QString target = QDir(dir).filePath(candidate.file.name);
                const QString part = target + QStringLiteral(".part");
                QFile::remove(part);
                if (!QDir().mkpath(dir) || !QFile::copy(path, part)) {
                    result.error = QStringLiteral("write");
                    break;
                }
                QFile::remove(target);
                if (!QFile::rename(part, target)) {
                    result.error = QStringLiteral("write");
                    break;
                }
                result = ImportResult{candidate.modelId, candidate.file.name, {}};
                break;
            }
        }
        if (!self)
            return;
        QMetaObject::invokeMethod(
            self.data(), [self, result]() {
                if (self)
                    self->finishImport(result);
            },
            Qt::QueuedConnection);
    });
}

void ModelManager::finishImport(const ImportResult& result)
{
    m_importing = false;
    emit importingChanged();
    const ModelInfo* info = model(result.modelId);
    if (result.error == QLatin1String("mismatch"))
        emit message(tr("The file has the expected size but its contents do not match any model. It was not imported."));
    else if (result.error == QLatin1String("read"))
        emit message(tr("The selected file could not be read."));
    else if (!result.error.isEmpty())
        emit message(tr("The model file could not be copied to the models folder."));
    else if (info && isInstalled(info->id))
        emit message(tr("%1 is ready to use.").arg(info->name));
    else if (info)
        emit message(tr("File imported. %1 still needs other files.").arg(info->name));
    changed(info ? info->capability : QString());
}

void ModelManager::changed(const QString& capability)
{
    ++m_revision;
    emit revisionChanged();
    reloadLists();
    if (!capability.isEmpty())
        emit activeModelChanged(capability);
}

void ModelManager::reloadLists()
{
    m_translationList->reload();
    m_speechToTextList->reload();
    m_textToSpeechList->reload();
    m_voiceTable->reload();
}

void ModelManager::refreshLists(const QString& id)
{
    m_translationList->refresh(id);
    m_speechToTextList->refresh(id);
    m_textToSpeechList->refresh(id);
    m_voiceTable->refresh(id);
}
