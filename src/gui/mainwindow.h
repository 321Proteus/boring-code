#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gui/view.hpp"
#include "gui/worker.hpp"
#include "core/loader.hpp"
#include "data/session.hpp"
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Session& sess, QWidget *parent = nullptr);
    ~MainWindow();
    void loadTraceAsync(QString path);
    void loadBinary(QString path, BCFileType type);

private slots:
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    Ui::MainWindow *ui;
    Session& session;
    QThread* worker_thread = nullptr;
    BCWorker* worker = nullptr;
    std::unique_ptr<QtViewModel> view;
};
#endif // MAINWINDOW_H
