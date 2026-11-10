#include "audio/AudioDsp.h"
#include "core/Announcement.h"
#include "core/AppController.h"
#include "core/AppSettings.h"
#include "engines/FrameParser.h"
#include "models/ModelCatalog.h"
#include "models/ModelManager.h"
#include "segmenter.hpp"
#include "spellcheck.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

#include <cmath>

namespace {

const QByteArray kHelloSha = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

QByteArray readTestData(const char* name)
{
    QFile file(QStringLiteral(DUCKTOR_SOURCE_DIR "/tests/data/") + QLatin1String(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QByteArray tinyCatalog()
{
    return readTestData("tiny_catalog.json");
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

// QSettings instances in the same process share a cache, so persistence is checked against the
// raw file, not through another QSettings.
QByteArray readRawIni(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
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

    void spellTokenizerSplitsWords()
    {
        const auto tokens = ducktor::spell::tokenize("Hello world");
        QCOMPARE(tokens.size(), std::size_t(2));
        QCOMPARE(tokens[0].utf8Start, std::size_t(0));
        QCOMPARE(tokens[0].utf8End, std::size_t(5));
        QCOMPARE(tokens[0].utf16Start, std::size_t(0));
        QCOMPARE(tokens[0].utf16End, std::size_t(5));
        QCOMPARE(tokens[1].utf8Start, std::size_t(6));
        QCOMPARE(tokens[1].utf8End, std::size_t(11));
        QCOMPARE(tokens[1].utf16Start, std::size_t(6));
        QCOMPARE(tokens[1].utf16End, std::size_t(11));
    }

    void spellTokenizerKeepsApostrophesAndHyphens()
    {
        // "l'ortografía" keeps its apostrophe, "co-autor" its hyphen, and the trailing
        // hyphen of "test-" is not part of a word. á makes the UTF-8 and UTF-16 offsets diverge.
        const auto tokens = ducktor::spell::tokenize("l'ortografía co-autor test-");
        QCOMPARE(tokens.size(), std::size_t(3));
        QCOMPARE(tokens[0].utf8Start, std::size_t(0));
        QCOMPARE(tokens[0].utf8End, std::size_t(13)); // 12 code points, one of them two bytes
        QCOMPARE(tokens[0].utf16Start, std::size_t(0));
        QCOMPARE(tokens[0].utf16End, std::size_t(12));
        QCOMPARE(tokens[1].utf8Start, std::size_t(14)); // "l'ortografía" is 13 bytes
        QCOMPARE(tokens[1].utf8End, std::size_t(22));
        QCOMPARE(tokens[1].utf16Start, std::size_t(13));
        QCOMPARE(tokens[1].utf16End, std::size_t(21));
        QCOMPARE(tokens[2].utf8Start, std::size_t(23));
        QCOMPARE(tokens[2].utf8End, std::size_t(27));
        QCOMPARE(tokens[2].utf16Start, std::size_t(22));
        QCOMPARE(tokens[2].utf16End, std::size_t(26));

        const auto leading = ducktor::spell::tokenize("'hello");
        QCOMPARE(leading.size(), std::size_t(1));
        QCOMPARE(leading[0].utf8Start, std::size_t(1)); // a leading apostrophe is not a word character
    }

    void spellTokenizerCountsUtf16Units()
    {
        // é is two bytes in UTF-8 but one UTF-16 unit; the emoji is one code point but two units.
        QCOMPARE(ducktor::spell::utf16Length("héllo"), std::size_t(5));
        QCOMPARE(ducktor::spell::utf16Length("a\U0001F600b"), std::size_t(4));

        const auto tokens = ducktor::spell::tokenize("a\U0001F600b");
        QCOMPARE(tokens.size(), std::size_t(2));
        QCOMPARE(tokens[0].utf8Start, std::size_t(0));
        QCOMPARE(tokens[0].utf8End, std::size_t(1));
        QCOMPARE(tokens[0].utf16Start, std::size_t(0));
        QCOMPARE(tokens[0].utf16End, std::size_t(1));
        QCOMPARE(tokens[1].utf8Start, std::size_t(5)); // "a" + the four-byte emoji
        QCOMPARE(tokens[1].utf8End, std::size_t(6));
        QCOMPARE(tokens[1].utf16Start, std::size_t(3));
        QCOMPARE(tokens[1].utf16End, std::size_t(4));
    }

    void spellTokenizerHandlesScriptsAndPunctuation()
    {
        const auto greek = ducktor::spell::tokenize("\xCE\xB3\xCE\xB5\xCE\xB9\xCE\xB1"); // γεια
        QCOMPARE(greek.size(), std::size_t(1));
        QCOMPARE(greek[0].utf8Length(), std::size_t(8)); // four code points, two bytes each
        QCOMPARE(greek[0].utf16Length(), std::size_t(4));

        const auto cjk = ducktor::spell::tokenize("hola \xE4\xBD\xA0\xE5\xA5\xBD world"); // hola 你好 world
        QCOMPARE(cjk.size(), std::size_t(3));
        QCOMPARE(cjk[1].utf8Start, std::size_t(5));
        QCOMPARE(cjk[1].utf8End, std::size_t(11));
        QCOMPARE(cjk[1].utf16Start, std::size_t(5));
        QCOMPARE(cjk[1].utf16End, std::size_t(7));

        const auto numbers = ducktor::spell::tokenize("abc123def");
        QCOMPARE(numbers.size(), std::size_t(2));
        QCOMPARE(numbers[0].utf8End, std::size_t(3));
        QCOMPARE(numbers[1].utf8Start, std::size_t(6));

        QVERIFY(ducktor::spell::tokenize("").empty());
        QVERIFY(ducktor::spell::tokenize("123 !!!").empty());
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
        const QByteArray json = readTestData("unsafe_catalog.json");
        QVERIFY(!json.isEmpty());
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
        ModelManager models(&settings, dir.path(), dir.filePath(QStringLiteral("models")), {});
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

    void controllerChecksSpellingEndToEnd()
    {
        // Full path through the helper process, the protocol and Hunspell; it only runs when
        // the spell-check engine is part of this build (ui-only skips it).
        const QString helper = QCoreApplication::applicationDirPath() + QStringLiteral("/ducktor-spellcheck");
        if (!QFileInfo(helper).isExecutable())
            QSKIP("the spell-check engine is not built into this tree");

        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        ModelManager models(&settings, dir.path(), dir.filePath(QStringLiteral("models")), {});
        AppController controller(&settings, &models);

        controller.checkSpelling(QStringLiteral("Hola mun do. La ortografia es im portante."), QStringLiteral("es"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 20000);

        const QVariantList issues = controller.spellingIssues();
        QCOMPARE(issues.size(), 3);
        const QVariantMap first = issues.at(0).toMap();
        QCOMPARE(first.value("word").toString(), QStringLiteral("mun"));
        QCOMPARE(first.value("start").toInt(), 5);
        QCOMPARE(first.value("end").toInt(), 8);
        const QVariantMap second = issues.at(1).toMap();
        QCOMPARE(second.value("word").toString(), QStringLiteral("ortografia"));
        QVERIFY(second.value("suggestions").toStringList().contains(QStringLiteral("ortografía")));
    }

    void managerTracksInstalledModels()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        const QString userDir = dir.filePath(QStringLiteral("models"));
        ModelManager models(&settings, dir.path(), userDir, {});
        QCOMPARE(models.dataDirectory(), dir.path());
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

    void managerSelectsVoiceGender()
    {
        QStringList warnings;
        QCOMPARE(ModelCatalog::parse(readTestData("voice_catalog.json"), &warnings).size(), 1);
        QCOMPARE(warnings.size(), 1);

        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        const QString userDir = dir.filePath(QStringLiteral("models"));
        ModelManager models(&settings, dir.path(), userDir, {});
        models.loadCatalog(readTestData("voice_catalog.json"));
        const QString id = QStringLiteral("two-voices");
        QVERIFY(models.voiceFor(QStringLiteral("es")).isEmpty());

        QVERIFY(writeFile(userDir + QStringLiteral("/two-voices/two.onnx"), "hello"));
        QCOMPARE(models.voiceFor(QStringLiteral("es")), id);
        models.selectVoice(QStringLiteral("es"), id, QStringLiteral("male"));
        QCOMPARE(models.voiceGender(QStringLiteral("es")), QStringLiteral("male"));
        QCOMPARE(models.voiceSpeaker(QStringLiteral("es")), 0);
        models.selectVoice(QStringLiteral("es"), id, QStringLiteral("female"));
        QCOMPARE(models.voiceSpeaker(QStringLiteral("es")), 1);
        QVERIFY(models.isVoiceActive(QStringLiteral("es"), id, QStringLiteral("female")));
        QVERIFY(!models.isVoiceActive(QStringLiteral("es"), id, QStringLiteral("male")));
        QCOMPARE(models.voiceTable()->rowCount(), 1);
    }

    void managerImportsOnlyMatchingFiles()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        ModelManager models(&settings, dir.path(), dir.filePath(QStringLiteral("models")), {});
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

    void announcementParsesAndLocalizes()
    {
        const QByteArray json = readTestData("announcement.json");
        QVERIFY(!json.isEmpty());

        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        QLocale::setDefault(QLocale("en"));
        Announcement announcement(&settings, json);
        QVERIFY(announcement.available());
        QCOMPARE(announcement.id(), QStringLiteral("test-announcement"));
        QCOMPARE(announcement.title(), QStringLiteral("Hello (en)"));
        QCOMPARE(announcement.body(), QStringLiteral("Body in English"));
        QVERIFY(announcement.unseen());

        QLocale::setDefault(QLocale("es"));
        announcement.relocalize();
        QCOMPARE(announcement.title(), QStringLiteral("Hola (es)"));
        QCOMPARE(announcement.body(), QStringLiteral("Cuerpo en español"));
    }

    void announcementFallsBackToEnglish()
    {
        const QByteArray json = readTestData("announcement.json");
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        QLocale::setDefault(QLocale("de")); // not present in the test file
        Announcement announcement(&settings, json);
        QVERIFY(announcement.available());
        QCOMPARE(announcement.title(), QStringLiteral("Hello (en)"));
    }

    void announcementAcceptsPlainText()
    {
        const QByteArray json = R"({"id":"plain","title":"Only title","body":"Only body"})";
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        QLocale::setDefault(QLocale("en"));
        Announcement announcement(&settings, json);
        QVERIFY(announcement.available());
        QCOMPARE(announcement.title(), QStringLiteral("Only title"));
        QCOMPARE(announcement.body(), QStringLiteral("Only body"));
    }

    void announcementRejectsInvalidData()
    {
        const QByteArray invalid[] = {
            QByteArray(),
            QByteArray("not json"),
            R"({"title":{"en":"t"},"body":{"en":"b"}})",
            R"({"id":"","title":{"en":"t"},"body":{"en":"b"}})",
            R"({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"url":"file://x"})",
            R"({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"urlLabel":{"en":"l"}})",
            R"js({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"url":"javascript:alert(1)","urlLabel":{"en":"l"}})js",
            R"({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"image":"//etc/passwd"})",
            R"({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"image":"icons/../../etc/passwd"})",
            R"({"id":"test-announcement","title":{"en":"t"},"body":{"en":"b"},"image":"icons/logo.png"})",
        };
        for (const QByteArray& json : invalid) {
            QTemporaryDir dir;
            AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
            Announcement announcement(&settings, json);
            QVERIFY2(!announcement.available(), qPrintable(json));
        }
    }

    void announcementPersistsAcknowledgement()
    {
        qApp->setApplicationVersion(QStringLiteral("1.2.3"));
        const QByteArray json = readTestData("announcement.json");
        const QString ini = QTemporaryDir().filePath(QStringLiteral("s.ini"));
        AppSettings settings(ini);
        QLocale::setDefault(QLocale("en"));
        Announcement announcement(&settings, json);
        QVERIFY(announcement.unseen());

        announcement.acknowledge();
        QVERIFY(!announcement.unseen());
        QCOMPARE(settings.value(QStringLiteral("ui/lastSeenAnnouncement")), QStringLiteral("test-announcement@1.2.3"));
        QVERIFY(readRawIni(ini).contains("lastSeenAnnouncement"));

        // A new instance over the same settings file keeps the acknowledgement.
        AppSettings reloaded(ini);
        Announcement reloadedAnnouncement(&reloaded, json);
        QVERIFY(!reloadedAnnouncement.unseen());

        // A different id is unseen again.
        const QByteArray other = R"({"id":"other-id","title":{"en":"t"},"body":{"en":"b"}})";
        Announcement otherAnnouncement(&reloaded, other);
        QVERIFY(otherAnnouncement.unseen());
    }

    void announcementReappearsInNewVersion()
    {
        const QByteArray json = readTestData("announcement.json");
        const QString ini = QTemporaryDir().filePath(QStringLiteral("s.ini"));
        QLocale::setDefault(QLocale("en"));

        qApp->setApplicationVersion(QStringLiteral("1.0.0"));
        AppSettings settings(ini);
        Announcement announcement(&settings, json);
        QVERIFY(announcement.unseen());
        announcement.acknowledge();
        QVERIFY(!announcement.unseen());
        QCOMPARE(settings.value(QStringLiteral("ui/lastSeenAnnouncement")), QStringLiteral("test-announcement@1.0.0"));

        // A new version shows the announcement again on its first launch.
        qApp->setApplicationVersion(QStringLiteral("1.1.0"));
        AppSettings reloaded(ini);
        Announcement newer(&reloaded, json);
        QVERIFY(newer.unseen());
        newer.acknowledge();
        QVERIFY(readRawIni(ini).contains("test-announcement@1.1.0"));
    }

    void bundledAnnouncementIsValid()
    {
        QFile file(QStringLiteral(DUCKTOR_SOURCE_DIR "/resources/announcement.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray json = file.readAll();
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("s.ini")));
        QLocale::setDefault(QLocale("en"));
        Announcement announcement(&settings, json);
        QVERIFY2(announcement.available(), qPrintable(json));
        QCOMPARE(announcement.id(), QStringLiteral("welcome-window-4"));
        QCOMPARE(announcement.title(), QStringLiteral("Ducktor"));
        QVERIFY(!announcement.body().isEmpty());
        QCOMPARE(announcement.image(), QStringLiteral("icons/ducktor.svg"));
    }

    void announcementTogglesVisibility()
    {
        const QByteArray json = readTestData("announcement.json");
        const QString ini = QTemporaryDir().filePath(QStringLiteral("s.ini"));
        AppSettings settings(ini);
        QLocale::setDefault(QLocale("en"));
        Announcement announcement(&settings, json);
        QVERIFY(announcement.unseen());

        // Acknowledging hides it; the inverse action shows it again on the next launch.
        announcement.acknowledge();
        QVERIFY(!announcement.unseen());
        QVERIFY(readRawIni(ini).contains("lastSeenAnnouncement"));
        announcement.showOnNextLaunch();
        QVERIFY(announcement.unseen());
        QVERIFY(!readRawIni(ini).contains("lastSeenAnnouncement"));

        // The reset is persisted, so a new instance over the same file is unseen again.
        AppSettings reloaded(ini);
        Announcement reloadedAnnouncement(&reloaded, json);
        QVERIFY(reloadedAnnouncement.unseen());

        // Calling it while already unseen is a no-op.
        announcement.showOnNextLaunch();
        QVERIFY(announcement.unseen());
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
