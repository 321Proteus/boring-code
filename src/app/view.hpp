#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"
#include <QListWidget>
#include <QTextBrowser>
#include <QProgressBar>

struct QtUI {
    QListWidget* trace_list;
    QTextBrowser* details_text;
    QProgressBar* progress_bar;
};

class QtViewModel :
    public BCTraceViewModel,
    public BCDetailsViewModel,
    public BCStatusViewModel
{
private:
    QtUI ui;
    std::pair<uint64_t, uint64_t> job_progress;
public:

    QtViewModel(const QtUI& ui) : ui(ui) {}

    void show_trace() const;
    void show_details(const BCBlock::Details& details) const;
    void show_details(const BCBasicBlock& details) const;
    void setup_job(uint64_t size);
    void update_job_progress(uint64_t new_progress);
    void show_error(const std::string& msg) const;
};
