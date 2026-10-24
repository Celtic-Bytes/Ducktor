#include "Languages.h"

#include <QCollator>
#include <QHash>
#include <QLocale>
#include <QVariantMap>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Languages {

const QList<LanguageInfo>& all()
{
    // Columns: code, English, Spanish, French, Italian, German, Chinese (prompt name).
    static const QList<LanguageInfo> languages{
        {u"es"_s, u"Spanish"_s, u"Español"_s, u"Espagnol"_s, u"Spagnolo"_s, u"Spanisch"_s, u"西班牙语"_s},
        {u"en"_s, u"English"_s, u"Inglés"_s, u"Anglais"_s, u"Inglese"_s, u"Englisch"_s, u"英语"_s},
        {u"fr"_s, u"French"_s, u"Francés"_s, u"Français"_s, u"Francese"_s, u"Französisch"_s, u"法语"_s},
        {u"de"_s, u"German"_s, u"Alemán"_s, u"Allemand"_s, u"Tedesco"_s, u"Deutsch"_s, u"德语"_s},
        {u"it"_s, u"Italian"_s, u"Italiano"_s, u"Italien"_s, u"Italiano"_s, u"Italienisch"_s, u"意大利语"_s},
        {u"pt"_s, u"Portuguese"_s, u"Portugués"_s, u"Portugais"_s, u"Portoghese"_s, u"Portugiesisch"_s, u"葡萄牙语"_s},
        {u"ca"_s, u"Catalan"_s, u"Catalán"_s, u"Catalan"_s, u"Catalano"_s, u"Katalanisch"_s, u"加泰罗尼亚语"_s},
        {u"nl"_s, u"Dutch"_s, u"Neerlandés"_s, u"Néerlandais"_s, u"Olandese"_s, u"Niederländisch"_s, u"荷兰语"_s},
        {u"pl"_s, u"Polish"_s, u"Polaco"_s, u"Polonais"_s, u"Polacco"_s, u"Polnisch"_s, u"波兰语"_s},
        {u"cs"_s, u"Czech"_s, u"Checo"_s, u"Tchèque"_s, u"Ceco"_s, u"Tschechisch"_s, u"捷克语"_s},
        {u"ru"_s, u"Russian"_s, u"Ruso"_s, u"Russe"_s, u"Russo"_s, u"Russisch"_s, u"俄语"_s},
        {u"uk"_s, u"Ukrainian"_s, u"Ucraniano"_s, u"Ukrainien"_s, u"Ucraino"_s, u"Ukrainisch"_s, u"乌克兰语"_s},
        {u"tr"_s, u"Turkish"_s, u"Turco"_s, u"Turc"_s, u"Turco"_s, u"Türkisch"_s, u"土耳其语"_s},
        {u"ar"_s, u"Arabic"_s, u"Árabe"_s, u"Arabe"_s, u"Arabo"_s, u"Arabisch"_s, u"阿拉伯语"_s},
        {u"he"_s, u"Hebrew"_s, u"Hebreo"_s, u"Hébreu"_s, u"Ebraico"_s, u"Hebräisch"_s, u"希伯来语"_s},
        {u"hi"_s, u"Hindi"_s, u"Hindi"_s, u"Hindi"_s, u"Hindi"_s, u"Hindi"_s, u"印地语"_s},
        {u"zh"_s, u"Chinese"_s, u"Chino"_s, u"Chinois"_s, u"Cinese"_s, u"Chinesisch"_s, u"中文"_s},
        {u"ja"_s, u"Japanese"_s, u"Japonés"_s, u"Japonais"_s, u"Giapponese"_s, u"Japanisch"_s, u"日语"_s},
        {u"ko"_s, u"Korean"_s, u"Coreano"_s, u"Coréen"_s, u"Coreano"_s, u"Koreanisch"_s, u"韩语"_s},
        {u"vi"_s, u"Vietnamese"_s, u"Vietnamita"_s, u"Vietnamien"_s, u"Vietnamita"_s, u"Vietnamesisch"_s, u"越南语"_s},
        {u"id"_s, u"Indonesian"_s, u"Indonesio"_s, u"Indonésien"_s, u"Indonesiano"_s, u"Indonesisch"_s, u"印尼语"_s},
        {u"th"_s, u"Thai"_s, u"Tailandés"_s, u"Thaï"_s, u"Thailandese"_s, u"Thailändisch"_s, u"泰语"_s},
    };
    return languages;
}

const QStringList& uiLanguages()
{
    static const QStringList codes{u"en"_s, u"es"_s, u"fr"_s, u"it"_s, u"de"_s};
    return codes;
}

QString uiLanguageName(const QString& code)
{
    static const QHash<QString, QString> names{{u"en"_s, u"English"_s},
                                               {u"es"_s, u"Español"_s},
                                               {u"fr"_s, u"Français"_s},
                                               {u"it"_s, u"Italiano"_s},
                                               {u"de"_s, u"Deutsch"_s}};
    return names.value(code, code);
}

QString uiLanguage()
{
    const QString code = QLocale().name().left(2);
    return uiLanguages().contains(code) ? code : uiLanguages().first();
}

const LanguageInfo* find(const QString& code)
{
    for (const LanguageInfo& info : all()) {
        if (info.code == code)
            return &info;
    }
    return nullptr;
}

QString displayName(const QString& code)
{
    const LanguageInfo* info = find(code);
    if (!info)
        return code;
    const QString ui = uiLanguage();
    if (ui == QLatin1String("es"))
        return info->nameEs;
    if (ui == QLatin1String("fr"))
        return info->nameFr;
    if (ui == QLatin1String("it"))
        return info->nameIt;
    if (ui == QLatin1String("de"))
        return info->nameDe;
    return info->nameEn;
}

QString promptName(const QString& code, const QString& promptLanguage)
{
    const LanguageInfo* info = find(code);
    if (!info)
        return {};
    return promptLanguage == QLatin1String("zh") ? info->nameZh : info->nameEn;
}

QVariantList options(const QStringList& codes)
{
    QList<const LanguageInfo*> known;
    for (const QString& code : codes) {
        if (const LanguageInfo* info = find(code))
            known << info;
    }
    QCollator collator;
    std::sort(known.begin(), known.end(), [&collator](const LanguageInfo* a, const LanguageInfo* b) {
        return collator.compare(displayName(a->code), displayName(b->code)) < 0;
    });
    QVariantList result;
    for (const LanguageInfo* info : known)
        result << QVariantMap{{QStringLiteral("code"), info->code}, {QStringLiteral("name"), displayName(info->code)}};
    return result;
}

} // namespace Languages
