#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>

struct LanguageInfo
{
    QString code;
    QString nameEs;
    QString nameEn;
    QString nameZh;
};

namespace Languages {

const QList<LanguageInfo>& all();
const LanguageInfo* find(const QString& code);
bool uiIsSpanish();
QString displayName(const QString& code);
// Name used inside model prompts; "zh" selects Chinese names, anything else English.
QString promptName(const QString& code, const QString& promptLanguage);
// [{code, name}] sorted by display name, restricted to known codes.
QVariantList options(const QStringList& codes);

} // namespace Languages
