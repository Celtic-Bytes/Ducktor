#include "Languages.h"

#include <QCollator>
#include <QLocale>
#include <QVariantMap>

#include <algorithm>

namespace Languages {

const QList<LanguageInfo>& all()
{
    static const QList<LanguageInfo> languages{
        {QStringLiteral("es"), QStringLiteral("Español"), QStringLiteral("Spanish"), QStringLiteral("西班牙语")},
        {QStringLiteral("en"), QStringLiteral("Inglés"), QStringLiteral("English"), QStringLiteral("英语")},
        {QStringLiteral("fr"), QStringLiteral("Francés"), QStringLiteral("French"), QStringLiteral("法语")},
        {QStringLiteral("de"), QStringLiteral("Alemán"), QStringLiteral("German"), QStringLiteral("德语")},
        {QStringLiteral("it"), QStringLiteral("Italiano"), QStringLiteral("Italian"), QStringLiteral("意大利语")},
        {QStringLiteral("pt"), QStringLiteral("Portugués"), QStringLiteral("Portuguese"), QStringLiteral("葡萄牙语")},
        {QStringLiteral("ca"), QStringLiteral("Catalán"), QStringLiteral("Catalan"), QStringLiteral("加泰罗尼亚语")},
        {QStringLiteral("nl"), QStringLiteral("Neerlandés"), QStringLiteral("Dutch"), QStringLiteral("荷兰语")},
        {QStringLiteral("pl"), QStringLiteral("Polaco"), QStringLiteral("Polish"), QStringLiteral("波兰语")},
        {QStringLiteral("cs"), QStringLiteral("Checo"), QStringLiteral("Czech"), QStringLiteral("捷克语")},
        {QStringLiteral("ru"), QStringLiteral("Ruso"), QStringLiteral("Russian"), QStringLiteral("俄语")},
        {QStringLiteral("uk"), QStringLiteral("Ucraniano"), QStringLiteral("Ukrainian"), QStringLiteral("乌克兰语")},
        {QStringLiteral("tr"), QStringLiteral("Turco"), QStringLiteral("Turkish"), QStringLiteral("土耳其语")},
        {QStringLiteral("ar"), QStringLiteral("Árabe"), QStringLiteral("Arabic"), QStringLiteral("阿拉伯语")},
        {QStringLiteral("he"), QStringLiteral("Hebreo"), QStringLiteral("Hebrew"), QStringLiteral("希伯来语")},
        {QStringLiteral("hi"), QStringLiteral("Hindi"), QStringLiteral("Hindi"), QStringLiteral("印地语")},
        {QStringLiteral("zh"), QStringLiteral("Chino"), QStringLiteral("Chinese"), QStringLiteral("中文")},
        {QStringLiteral("ja"), QStringLiteral("Japonés"), QStringLiteral("Japanese"), QStringLiteral("日语")},
        {QStringLiteral("ko"), QStringLiteral("Coreano"), QStringLiteral("Korean"), QStringLiteral("韩语")},
        {QStringLiteral("vi"), QStringLiteral("Vietnamita"), QStringLiteral("Vietnamese"), QStringLiteral("越南语")},
        {QStringLiteral("id"), QStringLiteral("Indonesio"), QStringLiteral("Indonesian"), QStringLiteral("印尼语")},
        {QStringLiteral("th"), QStringLiteral("Tailandés"), QStringLiteral("Thai"), QStringLiteral("泰语")},
    };
    return languages;
}

const LanguageInfo* find(const QString& code)
{
    for (const LanguageInfo& info : all()) {
        if (info.code == code)
            return &info;
    }
    return nullptr;
}

bool uiIsSpanish()
{
    return QLocale().language() == QLocale::Spanish;
}

QString displayName(const QString& code)
{
    const LanguageInfo* info = find(code);
    if (!info)
        return code;
    return uiIsSpanish() ? info->nameEs : info->nameEn;
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
