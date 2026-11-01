#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>

struct LanguageInfo
{
    QString code;
    QString nameEn;
    QString nameEs;
    QString nameFr;
    QString nameIt;
    QString nameDe;
    QString nameZh;
};

namespace Languages {

const QList<LanguageInfo>& all();
const LanguageInfo* find(const QString& code);
// Interface languages, in menu order; the first one is the default.
const QStringList& uiLanguages();
// Native name shown in the interface language menu.
QString uiLanguageName(const QString& code);
// Current interface language, taken from QLocale's default locale.
QString uiLanguage();
// Language name written in the current interface language.
QString displayName(const QString& code);
// Name used inside model prompts; "zh" selects Chinese names, anything else English.
QString promptName(const QString& code, const QString& promptLanguage);
// [{code, name}] sorted by display name, restricted to known codes.
QVariantList options(const QStringList& codes);

} // namespace Languages
