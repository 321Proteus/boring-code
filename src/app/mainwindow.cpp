#include "mainwindow.h"
#include <memory>
#include "./ui_mainwindow.h"
#include "../core/loader.hpp"
#include "../core/database.hpp"
#include "../core/block.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TraceView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    ui->DetailsView->setOpenLinks(false);

    connect(ui->TraceView, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);
}

void MainWindow::loadDatabase() {

    this->db = std::make_unique<BCDatabase>(load_database("functions.bin"));


    const int LIMIT = 100;
    int i = 0;

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
    if (selected.size() == 1) {
        QString block_name = selected.first()->text();
        BCBlock* block = db->getByName(block_name.toStdString());
        std::string details = block->info(db.get());
        ui->DetailsView->setText(QString::fromStdString(details));
    } else {
        ui->DetailsView->clear();
    }
}
