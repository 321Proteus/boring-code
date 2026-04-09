#include "view.hpp"
#include "trace_model.hpp"
#include "core/block.hpp"
#include <cstdint>
#include <QString>
#include <QMessageBox>
#include <qobject.h>

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

    QTreeWidgetItem* members_widget = ui.details_view->topLevelItem(4);
    members_widget->setText(0, "Locations");
    members_widget->setText(
        1,
        QString::number(details.members.size())
    );

    members_widget->takeChildren();

    QList<QTreeWidgetItem *> members;
    for (const BCAddr& loc : details.members) {

        QTreeWidgetItem* child = new QTreeWidgetItem();

        child->setForeground(0, Qt::darkBlue);
        QFont font = child->font(0);
        font.setUnderline(true);
        child->setFont(0, font);

        child->setText(0, QString::fromStdString(to_hex(loc)));
        members.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(loc));
    }

    members_widget->addChildren(members);

    QTreeWidgetItem* prevs_widget = ui.details_view->topLevelItem(5);
    prevs_widget->setText(0, "Predecessors");
    prevs_widget->setText(
        1,
        QString::number(details.prevs.size())
    );

    prevs_widget->takeChildren();

    QList<QTreeWidgetItem *> prevs;
    for (const Neighbor& prev : details.prevs) {
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, QString("%1 (x%2)").arg(prev.name).arg(prev.count));
        prevs.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(prev.name));
    }

    prevs_widget->addChildren(prevs);

    QTreeWidgetItem* nexts_widget = ui.details_view->topLevelItem(6);
    nexts_widget->setText(0, "Successors");
    nexts_widget->setText(
        1,
        QString::number(details.nexts.size())
    );

    nexts_widget->takeChildren();

    QList<QTreeWidgetItem *> nexts;
    for (const Neighbor& next : details.nexts) {
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, QString("%1 (x%2)").arg(next.name).arg(next.count));
        nexts.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(next.name));
    }

    nexts_widget->addChildren(nexts);

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
    QMessageBox::warning(this->ui.trace_view->parentWidget(), "BoringCode error", QString::fromStdString(msg));
}
