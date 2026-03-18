#include "mainwindow.h"
#include <memory>
#include "./ui_mainwindow.h"
#include "../core/loader.hpp"
#include "../core/database.hpp"
#include "../core/block.hpp"
#include "app/view.hpp"
#include "data/session.hpp"
#include "ui/view.hpp"
#include <iostream>

MainWindow::MainWindow(Session& sess, QWidget *parent)
    : QMainWindow(parent), session(sess)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TraceView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    ui->DetailsView->setOpenLinks(false);

    QtUI qtui {
        .trace_list = ui->TraceView,
        .details_text = ui->DetailsView,
        .progress_bar = nullptr
    };

    view = std::make_unique<QtView>(qtui);

    session.trace_view = view.get();
    session.details_view = view.get();
    session.status_view = view.get();

    connect(ui->TraceView, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);
}

void MainWindow::loadDatabase() {

    BCStatusView* sv = this->session.status_view;
    this->session.database = std::make_unique<BCDatabase>(load_database("functions.bin", *sv));

    BCDatabase* db = this->session.database.get();

    const int LIMIT = 100;
    int i = 0;
    std::cout << (db == nullptr) << std::endl;
    for (const auto& step : db->trace) {
        BCBlock* block = db->getById(step);
        ui->TraceView->addItem(QString::fromStdString(block->name));
        if (i++ == LIMIT) break;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSelectionChanged() {
    auto selected = ui->TraceView->selectedItems();
    BCDatabase* db = this->session.database.get();
    if (selected.size() == 1) {
        QString block_name = selected.first()->text();
        BCBlock* block = db->getByName(block_name.toStdString());
        this->session.details_view->show_details(db->generate_details(*block));
    } else {
        ui->DetailsView->clear();
    }
}
