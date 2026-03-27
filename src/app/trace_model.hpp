#pragma once

#include "core/database.hpp"
#include "ui/view.hpp"
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include <QVariant>
#include <memory>

inline std::shared_ptr<std::vector<BCTraceEntry>> precompute_trace(const BCDatabase& db, BCStatusViewModel& sv) {
    const auto& tr = db.trace;
    uint64_t size = tr.size();
    
    auto trace_list = std::make_shared<std::vector<BCTraceEntry>>();
    trace_list->reserve(size);

    sv.setup_job("Precomputing UI trace entries", size);
    for (int i=0;i<size;i++) {
        uint32_t id = tr[i];
        trace_list->push_back({ db.getById(id)->name, id });
        sv.update_job_progress(i);
    }
    sv.update_job_progress(size);
    return trace_list;
}

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

        BCTraceEntry e = entries->at(index.row());

        if (role == Qt::DisplayRole) return QString::fromStdString(e.name);
        if (role == Qt::UserRole) return e.id;

        return {};
    }

private:
    std::shared_ptr<std::vector<BCTraceEntry>> entries;
};