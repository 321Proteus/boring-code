#include "view.hpp"
#include "trace_model.hpp"
#include "core/block.hpp"
#include "core/util/util.hpp"
#include <cstdint>
#include <QString>

void QtViewModel::show_details(const BCBlock::Details& details) {

    QTreeWidgetItem* name_widget = ui.details_view->topLevelItem(0);
    name_widget->setText(0, "Name");
    name_widget->setText(1,
        QString::fromStdString(details.name)
    );

    QTreeWidgetItem* usage_count_widget = ui.details_view->topLevelItem(1);
    usage_count_widget->setText(0, "Usage count");
    usage_count_widget->setText(
        1,
        QString("%1 (top %2%)")
            .arg(details.usage_count.value)
            .arg(int(details.usage_count.percentile * 10000)/10000.0)
    );

    QTreeWidgetItem* instr_count_widget = ui.details_view->topLevelItem(2);
    instr_count_widget->setText(0, "Instruction count");
    instr_count_widget->setText(
        1,
        QString::fromStdString("TODO")
    );

    QTreeWidgetItem* loc_count_widget = ui.details_view->topLevelItem(3);
    loc_count_widget->setText(0, "Location count");
    loc_count_widget->setText(
        1,
        QString::fromStdString("TODO")
    );

    QTreeWidgetItem* size_widget = ui.details_view->topLevelItem(4);
    size_widget->setText(0, "Total size");
    size_widget->setText(
        1,
        QString::fromStdString("TODO")
    );

    QTreeWidgetItem* locs_widget = ui.details_view->topLevelItem(4);
    locs_widget->setText(0, "Locations");
    locs_widget->setText(
        1,
        QString::number(details.locs.size())
    );

    locs_widget->takeChildren();

    QList<QTreeWidgetItem *> locs;

    for (const BCAddr& loc : details.locs) {

        QTreeWidgetItem* child = new QTreeWidgetItem();

        child->setForeground(0, Qt::darkBlue);
        QFont font = child->font(0);
        font.setUnderline(true);
        child->setFont(0, font);

        child->setText(0, QString::fromStdString(to_hex(loc)));
        locs.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(loc));
    }

    locs_widget->addChildren(locs);

}

void QtViewModel::show_details(const BCBasicBlock& details)  {

}

void QtViewModel::show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const {
    auto model = new TraceModel(trace, ui.trace_view);
    ui.trace_view->setModel(model);
    ui.trace_view->setUniformItemSizes(true);
}

void QtViewModel::setup_job(const std::string name, uint64_t size) {
    job = { name, size, 0 };
    ui.progress_text->setText(QString::fromStdString(name));
    ui.progress_bar->setRange(0, size);
    ui.progress_bar->setValue(0);
}

void QtViewModel::update_job_progress(uint64_t progress) {
    ui.progress_bar->setValue(progress);
}

void QtViewModel::show_error(const std::string& msg) {

}
