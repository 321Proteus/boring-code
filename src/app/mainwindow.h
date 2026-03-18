#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "app/view.hpp"
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
    void loadDatabase();

private slots:
    void onSelectionChanged();

private:
    Ui::MainWindow *ui;
    Session& session;
    std::unique_ptr<QtViewModel> view;
};
#endif // MAINWINDOW_H
