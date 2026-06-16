#include "mainwindow.h"
#include <cassert>
#include <iostream>
#include <memory>
#include "./ui_mainwindow.h"
#include "core/address.hpp"
#include "core/database.hpp"
#include "core/loader.hpp"
#include "core/object.hpp"
#include "data/provider.hpp"
#include <capstone/capstone.h>
#include "view.hpp"
#include "worker.hpp"
#include "data/session.hpp"
#include "core/view.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QThread>
#include <QElapsedTimer>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QClipboard>
#include <QShortcut>
#include <string>
#include <vector>
#include <zlib.h>
#include "panel.hpp"

MainWindow::MainWindow(Session& sess, QWidget *parent)
    : QMainWindow(parent), session(sess)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TraceView->setSelectionMode(QAbstractItemView::ContiguousSelection);

    ui->CodeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->CodeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    
    setAcceptDrops(true);

    QShortcut* copyShortcut = new QShortcut(QKeySequence::Copy, ui->CodeView);

    connect(copyShortcut, &QShortcut::activated, this, [this]() {
        QStringList lines;

        for (QListWidgetItem* item : ui->CodeView->selectedItems()) {
            lines << item->text();
        }
        
        QApplication::clipboard()->setText(lines.join("\n"));
    });

    QtUI qtui {
        ui->TraceView,
        ui->DetailsView,
        ui->CodeView,
        ui->ProgressBar,
        ui->ProgressText
    };

    view = std::make_unique<QtViewModel>(qtui);

    session.trace_view = view.get();
    session.details_view = view.get();
    session.status_view = view.get();
    
    TracePanel* trace_panel = new TracePanel(ui->rightLayout, ui->TraceView);
    CodePanel* code_panel = new CodePanel(ui->leftLayout, ui->CodeView);

    if (session.has("path")) {
        loadTraceAsync(QString::fromStdString(session.get<std::string>("path")));
    }

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
    
    BCFileType type = detect_type(path.toStdString());
    
    switch (type) {

        case BCFileType::BCTRACE:
            loadTraceAsync(path);
            break;

        case BCFileType::UNKNOWN:
            session.status_view->show_error("The provided file is not a valid BoringCode trace!");
            break;
            
        default:
            session.status_view->show_error("TODO: Pending reimplementation");
            break;

    }

}

void MainWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {

    auto indexes = ui->TraceView->selectionModel()->selectedRows();
    BCDatabase* db = this->session.database.get();
    BCCodeProviderRegistry* prov_reg = this->session.provider_registry.get();

    if (indexes.size() >= 1) {

        ui->CodeView->clear();
        QList<QString> code;

        if (prov_reg != nullptr) {

            for (const QModelIndex& index : indexes) {

                BCObjectId id((quint64)index.data(Qt::UserRole).toULongLong());
                BCObject* object = db->store.get(id);
                std::vector<BCAddr> code_addrs = object->get_code_addrs();

                for (const BCAddr address : code_addrs) {

                    BCCodeProvider* prov = prov_reg->find(address);
                    if (prov != nullptr) {
                        std::vector<BCInstruction> bb = prov->get_bb(address);

                        for (const BCInstruction& in : bb) {
                            QListWidgetItem* el = new QListWidgetItem();
                            QString line = QString("%1\t%2\t%3").arg(to_hex(in.va), in.mnemonic, in.op_str);
                            code.append(line);
                        }
                    }

                }
            }
            
            ui->CodeView->addItems(code);

        }

        if (selected.size() == 1) {
            BCObjectId id((quint64)indexes.first().data(Qt::UserRole).toULongLong());
            BCObject* object = db->store.get(id);
            object->dispatch_details(*session.details_view);
        }
        
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

    connect(worker, &BCWorker::traceReady,
    this, [this](std::shared_ptr<std::vector<BCTraceEntry>> trace) {
        session.trace_view->show_trace(trace);
        connect(ui->TraceView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MainWindow::onSelectionChanged);
    });

    connect(worker, &BCWorker::finished,
        worker_thread, [this, timer]() {
            std::cout << "Worker thread quit peacefully after " << timer.elapsed() << " miliseconds" << std::endl;
            ui->ProgressBar->setValue(0);
            ui->ProgressText->setText("Idle");
            worker_thread->quit();  
        } );

    connect(worker_thread, &QThread::finished,
        worker, &QObject::deleteLater);

    connect(worker_thread, &QThread::finished,
        this, [timer]() { std::cout << "Thread finished\n"; });
    connect(worker_thread, &QThread::finished,
        worker_thread, &QObject::deleteLater);

    worker_thread->start();
}
