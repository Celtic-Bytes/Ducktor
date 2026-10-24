#include "VoiceTableModel.h"

#include "ModelManager.h"
#include "core/Languages.h"

#include <QCollator>
#include <QLocale>

#include <algorithm>

VoiceTableModel::VoiceTableModel(ModelManager* manager, QObject* parent)
    : QAbstractListModel(parent), m_manager(manager)
{
    reload();
}

int VoiceTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant VoiceTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows.at(index.row());
    switch (role) {
    case LanguageRole:
        return row.language;
    case LanguageNameRole:
        return Languages::displayName(row.language);
    case FemaleRole:
        return cell(row, QStringLiteral("female"));
    case MaleRole:
        return cell(row, QStringLiteral("male"));
    default:
        return {};
    }
}

QHash<int, QByteArray> VoiceTableModel::roleNames() const
{
    return {{LanguageRole, "language"}, {LanguageNameRole, "languageName"}, {FemaleRole, "female"}, {MaleRole, "male"}};
}

QVariantMap VoiceTableModel::cell(const Row& row, const QString& gender) const
{
    const QString& id = gender == QLatin1String("female") ? row.female : row.male;
    const ModelInfo* info = m_manager->model(id);
    if (!info)
        return {};
    return {{QStringLiteral("id"), info->id},
            {QStringLiteral("name"), info->name},
            {QStringLiteral("gender"), gender},
            {QStringLiteral("sizeText"), QLocale().formattedDataSize(info->totalSize())},
            {QStringLiteral("status"), m_manager->status(id)},
            {QStringLiteral("progress"), m_manager->progress(id)},
            {QStringLiteral("active"), m_manager->isVoiceActive(row.language, id, gender)},
            {QStringLiteral("shared"), info->voices.size() > 1},
            {QStringLiteral("locale"), info->locale},
            {QStringLiteral("quality"), info->quality},
            {QStringLiteral("license"), info->license},
            {QStringLiteral("licenseUrl"), info->licenseUrl},
            {QStringLiteral("homepage"), info->homepage}};
}

void VoiceTableModel::reload()
{
    beginResetModel();
    m_rows.clear();
    for (const ModelInfo& info : m_manager->models()) {
        if (info.capability != Capability::TextToSpeech)
            continue;
        auto row = std::find_if(m_rows.begin(), m_rows.end(),
                                [&info](const Row& candidate) { return candidate.language == info.voiceLanguage; });
        if (row == m_rows.end()) {
            m_rows.append(Row{info.voiceLanguage, {}, {}});
            row = m_rows.end() - 1;
        }
        // The first catalog entry for each language and gender wins.
        if (info.voice(QStringLiteral("female")) && row->female.isEmpty())
            row->female = info.id;
        if (info.voice(QStringLiteral("male")) && row->male.isEmpty())
            row->male = info.id;
    }
    QCollator collator;
    std::sort(m_rows.begin(), m_rows.end(), [&collator](const Row& a, const Row& b) {
        return collator.compare(Languages::displayName(a.language), Languages::displayName(b.language)) < 0;
    });
    endResetModel();
}

void VoiceTableModel::refresh(const QString& id)
{
    for (qsizetype i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).female == id || m_rows.at(i).male == id)
            emit dataChanged(index(int(i)), index(int(i)), {FemaleRole, MaleRole});
    }
}
