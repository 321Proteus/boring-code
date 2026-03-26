#include "mainwindow.h"
#include <iostream>
#include <memory>
#include "./ui_mainwindow.h"
#include "../core/database.hpp"
#include "../core/block.hpp"
#include "view.hpp"
#include "worker.hpp"
#include "data/session.hpp"
#include "ui/view.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QThread>
#include <QElapsedTimer>

MainWindow::MainWindow(Session& sess, QWidget *parent)
    : QMainWindow(parent), session(sess)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TraceView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    setAcceptDrops(true);

    QtUI qtui {
        .trace_view = ui->TraceView,
        .details_view = ui->DetailsView,
        .progress_bar = ui->ProgressBar,
        .progress_text = ui->ProgressText
    };

    view = std::make_unique<QtViewModel>(qtui);

    session.trace_view = view.get();
    session.details_view = view.get();
    session.status_view = view.get();

    connect(ui->TraceView, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    QString path = urls.first().toLocalFile();
    if (path.isEmpty()) return;
    loadTraceAsync(path);

}

void MainWindow::onSelectionChanged() {

    auto selected = ui->TraceView->selectedItems();
    BCDatabase* db = this->session.database.get();

    if (selected.size() == 1) {

        QString block_name = selected.first()->text();
        BCBlock* block = db->getByName(block_name.toStdString());
        this->session.details_view->show_details(db->generate_details(*block));

    } else {

        for (int i=0;i<ui->DetailsView->topLevelItemCount();i++) {
            auto item = ui->DetailsView->topLevelItem(i);
            item->takeChildren();
            item->setText(1, "");
        }
    }
}

void MainWindow::loadTraceAsync(QString path) {
    if (worker_thread) {
        // worker->request_cancel();
        // worker_thread->quit();
        // worker_thread->wait();
    }
    QtStatusProxy* proxy = new QtStatusProxy();
    QElapsedTimer timer;
    timer.start();
    connect(proxy, &QtStatusProxy::jobSetup,
        this, [this](QString name, uint64_t size) {
            view->setup_job(name.toStdString(), size);
        });
    connect(proxy, &QtStatusProxy::jobProgress,
        this, [this](uint64_t progress) {
            view->update_job_progress(progress);
        });

    worker_thread = new QThread;
    worker = new BCWorker(session, path, proxy);
    worker->moveToThread(worker_thread);

    connect(worker_thread, &QThread::started,
        worker, &BCWorker::load_trace);

    connect(worker, &BCWorker::finished,
        worker_thread, [this, timer]() {
            std::cout << "Worker thread quit peacefully after " << timer.elapsed() << " miliseconds" << std::endl;
            ui->ProgressBar->setValue(0);
            ui->ProgressText->setText("Idle");
            session.trace_view->show_trace();
            worker_thread->quit();  
        } );

    connect(worker_thread, &QThread::finished,
        worker, &QObject::deleteLater);

    connect(worker_thread, &QThread::finished,
        this, [timer]() { std::cout << "Thread finished "; });
    connect(worker_thread, &QThread::finished,
        worker_thread, &QObject::deleteLater);

    worker_thread->start();
}
