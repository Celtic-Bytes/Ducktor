#include "ModelListModel.h"

#include "ModelManager.h"
#include "core/Languages.h"

#include <QLocale>

ModelListModel::ModelListModel(ModelManager* manager, const QString& capability, QObject* parent)
    : QAbstractListModel(parent), m_manager(manager), m_capability(capability)
{
    reload();
}

int ModelListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_ids.size());
}

QVariant ModelListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_ids.size())
        return {};
    const QString& id = m_ids.at(index.row());
    const ModelInfo* info = m_manager->model(id);
    if (!info)
        return {};

    switch (role) {
    case IdRole:
        return info->id;
    case NameRole:
        return info->name;
    case DescriptionRole:
        return info->description();
    case LicenseRole:
        return info->license;
    case LicenseUrlRole:
        return info->licenseUrl;
    case HomepageRole:
        return info->homepage;
    case ReleasedRole:
        return info->released;
    case SizeRole:
        return info->available ? QLocale().formattedDataSize(info->totalSize()) : QString();
    case LanguagesRole: {
        if (!info->voiceLanguage.isEmpty())
            return Languages::displayName(info->voiceLanguage);
        QStringList names;
        for (const QString& code : info->languages)
            names << Languages::displayName(code);
        return names.join(QStringLiteral(", "));
    }
    case StatusRole:
        return m_manager->status(id);
    case ProgressRole:
        return m_manager->progress(id);
    case ActiveRole:
        return m_manager->isActive(id);
    case RecommendedRole:
        return info->recommended;
    case ExperimentalRole:
        return info->experimental;
    case VoiceLanguageRole:
        return info->voiceLanguage;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModelListModel::roleNames() const
{
    return {{IdRole, "modelId"},
            {NameRole, "name"},
            {DescriptionRole, "description"},
            {LicenseRole, "license"},
            {LicenseUrlRole, "licenseUrl"},
            {HomepageRole, "homepage"},
            {ReleasedRole, "released"},
            {SizeRole, "sizeText"},
            {LanguagesRole, "languagesText"},
            {StatusRole, "status"},
            {ProgressRole, "progress"},
            {ActiveRole, "active"},
            {RecommendedRole, "recommended"},
            {ExperimentalRole, "experimental"},
            {VoiceLanguageRole, "voiceLanguage"}};
}

void ModelListModel::reload()
{
    beginResetModel();
    m_ids.clear();
    for (const ModelInfo& info : m_manager->models()) {
        if (info.capability == m_capability)
            m_ids << info.id;
    }
    endResetModel();
}

void ModelListModel::refresh(const QString& id)
{
    const qsizetype row = m_ids.indexOf(id);
    if (row < 0)
        return;
    const QModelIndex changed = index(int(row));
    emit dataChanged(changed, changed, {StatusRole, ProgressRole, ActiveRole});
}
