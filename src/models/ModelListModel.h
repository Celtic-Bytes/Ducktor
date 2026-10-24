#pragma once

#include <QAbstractListModel>
#include <QStringList>

class ModelManager;

class ModelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        LicenseRole,
        LicenseUrlRole,
        HomepageRole,
        ReleasedRole,
        SizeRole,
        LanguagesRole,
        StatusRole,
        ProgressRole,
        ActiveRole,
        RecommendedRole,
        ExperimentalRole,
        VoiceLanguageRole,
    };

    ModelListModel(ModelManager* manager, const QString& capability, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reload();
    void refresh(const QString& id);

private:
    ModelManager* m_manager;
    QString m_capability;
    QStringList m_ids;
};
