#include "mainwindow.h"
#include <cassert>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include "./ui_mainwindow.h"
#include "core/address.hpp"
#include "core/database.hpp"
#include "core/loader.hpp"
#include "core/object.hpp"
#include "data/provider.hpp"
#include "providers/direct.hpp"
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
#include <qobject.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <zlib.h>

MainWindow::MainWindow(Session& sess, QWidget *parent)
    : QMainWindow(parent), session(sess)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TraceView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    setAcceptDrops(true);

    QtUI qtui {
        ui->TraceView,
        ui->DetailsView,
        ui->ProgressBar,
        ui->ProgressText
    };

    view = std::make_unique<QtViewModel>(qtui);

    session.trace_view = view.get();
    session.details_view = view.get();
    session.status_view = view.get();
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
            loadBinary(path, type);
            break;

    }

}

void MainWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {

    auto indexes = ui->TraceView->selectionModel()->selectedIndexes();
    BCDatabase* db = this->session.database.get();
    BCCodeProvider* prov = this->session.code_provider.get();

    if (selected.size() == 1) {

        QModelIndex index = indexes.first();
        uint32_t id = index.data(Qt::UserRole).toUInt();
        BCObject* object = db->resolve_object(id);
        object->dispatch_details(*session.details_view);

        if (prov != nullptr) {

            ui->CodeView->clear();
            QList<QString> code;

            std::vector<BCAddr> code_addrs = object->get_code_addrs();

            for (const BCAddr address : code_addrs) {

                std::vector<BCInstruction> bb = prov->get_bb(db->base_address, address);

                for (const BCInstruction& in : bb) {
                    QListWidgetItem* el = new QListWidgetItem();
                    QString line = QString("%1\t%2\t%3").arg(to_hex(in.va), in.mnemonic, in.op_str);
                    code.append(line);
                }
            }
            
            ui->CodeView->addItems(code);

        }

    } else {
        for (int i=0;i<ui->DetailsView->topLevelItemCount();i++) {
            auto item = ui->DetailsView->topLevelItem(i);
            item->takeChildren();
            item->setText(1, "");
        }
    }
}

void MainWindow::loadBinary(QString path, BCFileType type) {
    std::ifstream f(path.toStdString(), std::ios::binary);
    uint32_t crc = 0;
    unsigned char buf[8192];

    while (f.read((char*)buf, sizeof(buf))) crc = crc32(crc, buf, f.gcount());
    crc = crc32(crc, buf, f.gcount());
    f.close();

    if (session.database == nullptr) {
        session.checksum = crc;
    } else {
        if (session.checksum != crc) {
            session.status_view->show_error("The file checksum doesn't match!");
            return;
        }
    }

    auto mapped_file = std::make_shared<MappedFile>(path.toStdString());
    printf("File size: %zu\n", mapped_file.get()->size());

    auto provider = std::make_unique<DirectCodeProvider>(mapped_file);

    try {
        if (type == BCFileType::PE)         provider->parse_pe();
        else if (type == BCFileType::ELF)   provider->parse_elf();
        else {
            session.status_view->show_error("Unrecognized file type");
            return;
        }
    } catch (const std::runtime_error& e) {
        session.status_view->show_error(e.what());
        return;
    }

    provider->init_cs_handler();
    session.code_provider = std::move(provider);
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
        this, [timer]() { std::cout << "Thread finished "; });
    connect(worker_thread, &QThread::finished,
        worker_thread, &QObject::deleteLater);

    worker_thread->start();
}
