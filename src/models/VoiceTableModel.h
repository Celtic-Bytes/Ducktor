#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantMap>

class ModelManager;

// One row per language with its female and male voice, for the compact voice settings table.
class VoiceTableModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        LanguageRole = Qt::UserRole + 1,
        LanguageNameRole,
        FemaleRole,
        MaleRole,
    };

    explicit VoiceTableModel(ModelManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reload();
    void refresh(const QString& id);

private:
    struct Row
    {
        QString language;
        QString female;
        QString male;
    };

    QVariantMap cell(const Row& row, const QString& gender) const;

    ModelManager* m_manager;
    QList<Row> m_rows;
};
