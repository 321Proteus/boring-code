#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"
#include <QListWidget>
#include <QTreeWidget>
#include <QProgressBar>
#include <QString>
#include <QLabel>

struct QtUI {
    QListWidget* trace_view;
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

    void show_trace() const;
    void show_details(const BCBlock::Details& details);
    void show_details(const BCBasicBlock& details);
    void setup_job(const std::string name, uint64_t size);
    void update_job_progress(uint64_t new_progress);
    void show_error(const std::string& msg) const;
};
