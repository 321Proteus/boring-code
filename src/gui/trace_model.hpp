#pragma once

#include "core/view.hpp"
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include <QVariant>
#include <memory>

class TraceModel : public QAbstractListModel {
    Q_OBJECT;
public:
    TraceModel(std::shared_ptr<std::vector<BCTraceEntry>> entries, QObject* parent = nullptr)
        : QAbstractListModel(parent), entries(entries) {}
        
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return entries->size();
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid())
            return {};

        const BCTraceEntry& e = entries->at(index.row());

        if (role == Qt::DisplayRole) return QString::fromStdString(e.name);
        if (role == Qt::UserRole) return e.id;

        return {};
    }

private:
    std::shared_ptr<std::vector<BCTraceEntry>> entries;
};