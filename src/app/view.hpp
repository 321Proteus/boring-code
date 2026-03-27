#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"
#include <QListView>
#include <QTreeWidget>
#include <QProgressBar>
#include <QString>
#include <QLabel>

struct QtUI {
    QListView* trace_view;
    QTreeWidget* details_view;
    QProgressBar* progress_bar;
    QLabel* progress_text;
};

class QtViewModel :
    public BCTraceViewModel,
    public BCDetailsViewModel,
    public BCStatusViewModel
{
private:
    QtUI ui;
    BCJob job;
public:

    QtViewModel(const QtUI& ui) : ui(ui) {}

    void show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const;
    void show_details(const BCBlock::Details& details);
    void show_details(const BCBasicBlock& details);
    void setup_job(const std::string name, uint64_t size);
    void update_job_progress(uint64_t new_progress);
    void show_error(const std::string& msg);
};

class QtStatusProxy :
    public QObject,
    public BCStatusViewModel
{
    Q_OBJECT
private:
    BCJob job;
    int last_percent = -1;
public:

    void setup_job(const std::string name, uint64_t size) override {
        job = { name, size, 0 };
        emit jobSetup(QString::fromStdString(name), size);
    }
    void update_job_progress(uint64_t progress) override {
        if (!job.size) return;
        int percent = (progress * 100) / job.size;
        if (percent != last_percent) {
            last_percent = percent;
            emit jobProgress(progress);
        }
    }
    void show_error(const std::string& msg) override {
        emit jobError(QString::fromStdString(msg));
    }

signals:
    void jobSetup(QString name, uint64_t size);
    void jobProgress(uint64_t progress);
    void jobError(QString msg);
};
