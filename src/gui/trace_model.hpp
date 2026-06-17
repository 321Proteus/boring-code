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
        if (parent.isValid()) return 0;
        return entries->size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return 2;
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid())
            return {};

        const BCTraceEntry& e = entries->at(index.row());

        if (role == Qt::DisplayRole) {
            if (index.column() == 0) return QString::fromStdString(e.name);
            else if (index.column() == 1) return QString::fromStdString(e.module);
        }
        if (role == Qt::UserRole) return QVariant((quint64)e.id.raw);

        return {};
    }

private:
    std::shared_ptr<std::vector<BCTraceEntry>> entries;
};