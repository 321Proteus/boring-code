#include "view.hpp"
#include "core/object.hpp"
#include "trace_model.hpp"
#include <cstdint>
#include <QString>
#include <QMessageBox>
#include <qobject.h>

void show_object_info(QTreeWidget& widget, const BCObject& object) {

    QTreeWidgetItem* name_widget = new QTreeWidgetItem(&widget);
    name_widget->setText(0, "Name");
    name_widget->setText(1,
        QString::fromStdString(object.name)
    );
    
    QTreeWidgetItem* usage_count_widget = new QTreeWidgetItem(&widget);
    usage_count_widget->setText(0, "Usage count");
    auto uc = object.usage_count;

    if (uc.percentile == -1)
        usage_count_widget->setText(1, QString::number(uc.value));
    else
        usage_count_widget->setText(
            1,
            QString("%1 (top %2%)")
                .arg(uc.value)
                .arg(int(uc.percentile * 10000)/10000.0)
        );

    QTreeWidgetItem* prevs_widget = new QTreeWidgetItem(&widget);
    prevs_widget->setText(0, "Predecessors");
    prevs_widget->setText(
        1,
        QString::number(object.prevs.size())
    );
    prevs_widget->takeChildren();
    QList<QTreeWidgetItem *> prevs;

    for (const Neighbor& prev : object.prevs) {
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, QString("%1 (x%2)").arg(prev.name).arg(prev.count));
        prevs.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(prev.name));
    }

    prevs_widget->addChildren(prevs);

    QTreeWidgetItem* nexts_widget = new QTreeWidgetItem(&widget);
    nexts_widget->setText(0, "Successors");
    nexts_widget->setText(
        1,
        QString::number(object.nexts.size())
    );
    nexts_widget->takeChildren();
    QList<QTreeWidgetItem *> nexts;

    for (const Neighbor& next : object.nexts) {
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0, QString("%1 (x%2)").arg(next.name).arg(next.count));
        nexts.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(next.name));
    }

    nexts_widget->addChildren(nexts);

}

void QtViewModel::show_details(const BCBlock& block) const {

    ui.details_view->clear();

    show_object_info(*ui.details_view, block);

    QTreeWidgetItem* instr_count_widget = new QTreeWidgetItem(ui.details_view);
    instr_count_widget->setText(0, "Instruction count");
    instr_count_widget->setText(1, "TODO");

    QTreeWidgetItem* size_widget = new QTreeWidgetItem(ui.details_view);
    size_widget->setText(0, "Total size");
    size_widget->setText(1, "TODO");

    QTreeWidgetItem* members_widget = new QTreeWidgetItem(ui.details_view);
    members_widget->setText(0, "Locations");
    members_widget->setText(1, QString::number(block.members.size()));

    members_widget->takeChildren();

    QList<QTreeWidgetItem *> members;
    for (const BCObject* m : block.members) {

        QTreeWidgetItem* child = new QTreeWidgetItem();

        child->setForeground(0, Qt::darkBlue);
        QFont font = child->font(0);
        font.setUnderline(true);
        child->setFont(0, font);

        child->setText(0, QString::fromStdString(m->name));
        members.append(child);
        child->setData(0, Qt::UserRole, QVariant::fromValue(m->id));
    }

    members_widget->addChildren(members);

}

void QtViewModel::show_details(const BCBasicBlock& bb) const {

    ui.details_view->clear();

    show_object_info(*ui.details_view, bb);

}

void QtViewModel::show_details(const BCLoop& loop) const {

    ui.details_view->clear();

    show_object_info(*ui.details_view, loop);

    QTreeWidgetItem* body_widget = new QTreeWidgetItem(ui.details_view);
    body_widget->setText(0, "Members");
    body_widget->setText(1, QString::number(loop.body.size()));

    body_widget->takeChildren();

    QList<QTreeWidgetItem *> body;
    for (const BCObject* m : loop.body) {

        QTreeWidgetItem* el = new QTreeWidgetItem();

        el->setForeground(0, Qt::darkBlue);
        QFont font = el->font(0);
        font.setUnderline(true);
        el->setFont(0, font);

        el->setText(0, QString::fromStdString(m->name));
        body.append(el);
        el->setData(0, Qt::UserRole, QVariant::fromValue(m->id));
    }

    body_widget->addChildren(body);

}

void QtViewModel::show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const {
    auto model = new TraceModel(trace, ui.trace_view);
    ui.trace_view->setModel(model);
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
