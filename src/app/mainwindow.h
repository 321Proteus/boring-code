#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "app/view.hpp"
#include "app/worker.hpp"
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

private slots:
    void onSelectionChanged();

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
