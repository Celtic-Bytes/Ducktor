#include "audio/AudioDsp.h"
#include "core/AppController.h"
#include "core/AppSettings.h"
#include "engines/FrameParser.h"
#include "models/ModelCatalog.h"
#include "models/ModelManager.h"
#include "segmenter.hpp"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

#include <cmath>

namespace {

const QByteArray kHelloSha = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

QByteArray tinyCatalog()
{
    return QByteArrayLiteral(R"({"models": [{
        "id": "tiny-model", "capability": "speech-to-text", "engine": "whisper", "name": "Tiny",
        "files": [{"name": "tiny.bin", "url": "https://huggingface.co/x/y/resolve/abc/tiny.bin",
                   "size": 5, "sha256": "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"}]}]})");
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

QString reassemble(const std::vector<ducktor::seg::Segment>& segments)
{
    std::string out;
    for (const auto& s : segments)
        out += s.prefix + s.text + s.suffix;
    return QString::fromStdString(out);
}

} // namespace

class TestCore : public QObject
{
    Q_OBJECT

private slots:
    void segmenterKeepsLayout()
    {
        QVERIFY(ducktor::seg::split("", 1500).empty());
        const std::string text = "  Hola mundo.  \n\nAdiós";
        const auto segments = ducktor::seg::split(text, 1500);
        QCOMPARE(reassemble(segments), QString::fromStdString(text));
        QCOMPARE(segments.size(), std::size_t(3));
        QVERIFY(segments[0].translate);
        QCOMPARE(segments[0].text, std::string("Hola mundo."));
        QVERIFY(!segments[1].translate);
    }

    void segmenterSplitsLongText()
    {
        std::string text;
        for (int i = 0; i < 60; ++i)
            text += "Esta es la frase número " + std::to_string(i) + ". ";
        const auto segments = ducktor::seg::split(text, 200);
        QVERIFY(segments.size() > 1);
        for (const auto& s : segments) {
            QVERIFY(s.translate);
            QVERIFY(s.text.size() <= 200);
            QVERIFY(!s.text.empty());
        }
    }

    void segmenterRespectsUtf8()
    {
        std::string text;
        for (int i = 0; i < 1000; ++i)
            text += "ñ";
        const auto parts = ducktor::seg::splitLong(text, 101);
        std::size_t total = 0;
        for (const std::string& part : parts) {
            QVERIFY(part.size() <= 101);
            QVERIFY((static_cast<unsigned char>(part.front()) & 0xC0) != 0x80);
            QCOMPARE(QString::fromUtf8(part.data(), qsizetype(part.size())).toUtf8().size(), qsizetype(part.size()));
            total += part.size();
        }
        QCOMPARE(total, text.size());
    }

    void frameParserHandlesFragments()
    {
        const QByteArray data = encodeFrame({"RESULT", "7", "1"}, "hola") + encodeFrame({"READY", "0"}, {});
        FrameParser parser;
        QList<EngineFrame> frames;
        for (const char c : data)
            QVERIFY(parser.feed(QByteArray(1, c), frames));
        QCOMPARE(frames.size(), 2);
        QCOMPARE(frames[0].command(), QByteArray("RESULT"));
        QCOMPARE(frames[0].field(1), QByteArray("7"));
        QCOMPARE(frames[0].payload, QByteArray("hola"));
        QCOMPARE(frames[1].command(), QByteArray("READY"));
    }

    void frameParserRejectsAbuse()
    {
        QList<EngineFrame> frames;
        FrameParser longHeader;
        QVERIFY(!longHeader.feed(QByteArray(400, 'A'), frames));
        FrameParser hugePayload;
        QVERIFY(!hugePayload.feed("AUDIO 1 99999999999\n", frames));
        FrameParser badSize;
        QVERIFY(!badSize.feed("AUDIO 1 abc\n", frames));
        QVERIFY(frames.isEmpty());
    }

    void bundledCatalogIsValid()
    {
        QFile file(QStringLiteral(DUCKTOR_SOURCE_DIR "/resources/models.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QStringList warnings;
        const QList<ModelInfo> models = ModelCatalog::parse(file.readAll(), &warnings);
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
        for (const QString& capability : {Capability::Translation, Capability::SpeechToText, Capability::TextToSpeech}) {
            bool hasAvailable = false;
            for (const ModelInfo& info : models)
                hasAvailable = hasAvailable || (info.capability == capability && info.available);
            QVERIFY2(hasAvailable, qPrintable(capability));
        }
    }

    void catalogRejectsUnsafeEntries()
    {
        const QByteArray json = QByteArrayLiteral(R"({"models": [
            {"id": "a1", "capability": "translation", "engine": "llama", "name": "A",
             "files": [{"name": "../evil.gguf", "url": "https://huggingface.co/a", "size": 1, "sha256": "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"}]},
            {"id": "b1", "capability": "translation", "engine": "llama", "name": "B",
             "files": [{"name": "b.gguf", "url": "http://huggingface.co/b", "size": 1, "sha256": "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"}]},
            {"id": "c1", "capability": "translation", "engine": "llama", "name": "C",
             "files": [{"name": "c.gguf", "url": "https://example.com/c", "size": 1, "sha256": "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"}]},
            {"id": "d1", "capability": "translation", "engine": "llama", "name": "D",
             "files": [{"name": "d.gguf", "url": "https://huggingface.co/d", "size": 1, "sha256": "xyz"}]},
            {"id": "../e", "capability": "translation", "engine": "llama", "name": "E", "files": []}
        ]})");
        QStringList warnings;
        QVERIFY(ModelCatalog::parse(json, &warnings).isEmpty());
        QCOMPARE(warnings.size(), 5);
    }

    void settingsClampLimits()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        QCOMPARE(settings.maxInputChars(), AppSettings::kHardMaxInputChars);
        settings.setMaxInputChars(3'000'000);
        QCOMPARE(settings.maxInputChars(), AppSettings::kHardMaxInputChars);
        settings.setMaxInputChars(1);
        QCOMPARE(settings.maxInputChars(), AppSettings::kMinInputChars);
        settings.setMaxRecordingSeconds(20 * 3600);
        QCOMPARE(settings.maxRecordingSeconds(), AppSettings::kHardMaxRecordingSeconds);
    }

    void controllerEnforcesTextLimit()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        ModelManager models(&settings, dir.filePath(QStringLiteral("models")), {});
        AppController controller(&settings, &models);

        QVERIFY(controller.fitsLimit(4990, 0, 10, 5000));
        QVERIFY(!controller.fitsLimit(4990, 0, 11, 5000));
        QVERIFY(controller.fitsLimit(5000, 100, 100, 5000));
        QVERIFY(!controller.fitsLimit(0, 0, 3'000'000, 5000));

        QSignalSpy notices(&controller, &AppController::notice);
        controller.translate(QString(5001, QLatin1Char('a')), QStringLiteral("auto"), QStringLiteral("en"));
        QCOMPARE(notices.size(), 1);
        QVERIFY(!controller.busy());
        controller.translate(QStringLiteral("hola"), QStringLiteral("auto"), QStringLiteral("en"));
        QCOMPARE(notices.size(), 2);
        QVERIFY(!controller.busy());
    }

    void managerTracksInstalledModels()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        const QString userDir = dir.filePath(QStringLiteral("models"));
        ModelManager models(&settings, userDir, {});
        models.loadCatalog(tinyCatalog());

        QVERIFY(!models.isInstalled(QStringLiteral("tiny-model")));
        QCOMPARE(models.status(QStringLiteral("tiny-model")), QStringLiteral("notInstalled"));
        QVERIFY(models.activeModelId(Capability::SpeechToText).isEmpty());

        QVERIFY(writeFile(userDir + QStringLiteral("/tiny-model/tiny.bin"), "hello"));
        QVERIFY(models.isInstalled(QStringLiteral("tiny-model")));
        QCOMPARE(models.activeModelId(Capability::SpeechToText), QStringLiteral("tiny-model"));

        QSignalSpy changed(&models, &ModelManager::activeModelChanged);
        models.remove(QStringLiteral("tiny-model"));
        QVERIFY(!models.isInstalled(QStringLiteral("tiny-model")));
        QCOMPARE(changed.size(), 1);
    }

    void managerImportsOnlyMatchingFiles()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        ModelManager models(&settings, dir.filePath(QStringLiteral("models")), {});
        models.loadCatalog(tinyCatalog());

        const QString wrong = dir.filePath(QStringLiteral("wrong.bin"));
        QVERIFY(writeFile(wrong, "HELLO"));
        QSignalSpy messages(&models, &ModelManager::message);
        models.importFile(QUrl::fromLocalFile(wrong));
        QTRY_VERIFY(!models.importing());
        QVERIFY(!models.isInstalled(QStringLiteral("tiny-model")));

        const QString right = dir.filePath(QStringLiteral("right.bin"));
        QVERIFY(writeFile(right, "hello"));
        models.importFile(QUrl::fromLocalFile(right));
        QTRY_VERIFY(!models.importing());
        QVERIFY(models.isInstalled(QStringLiteral("tiny-model")));
        QCOMPARE(messages.size(), 2);
    }

    void resamplerKeepsDuration()
    {
        std::vector<float> input(48000);
        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = float(std::sin(double(i) * 0.01));
        AudioDsp::LinearResampler resampler(48000, 16000);
        std::vector<float> output;
        for (std::size_t i = 0; i < input.size(); i += 480)
            resampler.process(input.data() + i, 480, output);
        QVERIFY(std::abs(int(output.size()) - 16000) <= 2);
    }

    void monoConversionAndSilence()
    {
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        QByteArray pcm(8, '\0');
        qToLittleEndian<qint16>(16384, pcm.data());
        qToLittleEndian<qint16>(-16384, pcm.data() + 2);
        qToLittleEndian<qint16>(16384, pcm.data() + 4);
        qToLittleEndian<qint16>(16384, pcm.data() + 6);
        std::vector<float> mono;
        AudioDsp::appendMono(pcm.constData(), pcm.size(), format, mono);
        QCOMPARE(mono.size(), std::size_t(2));
        QVERIFY(std::abs(mono[0]) < 1e-6f);
        QVERIFY(std::abs(mono[1] - 0.5f) < 1e-6f);

        QVERIFY(AudioDsp::isProbablySilence(std::vector<float>(16000, 0.0f)));
        std::vector<float> tone(16000);
        for (std::size_t i = 0; i < tone.size(); ++i)
            tone[i] = 0.3f * float(std::sin(double(i) * 0.1));
        QVERIFY(!AudioDsp::isProbablySilence(tone));
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "tst_core.moc"
