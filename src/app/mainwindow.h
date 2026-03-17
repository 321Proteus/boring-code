#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/database.hpp"
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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadDatabase();
    std::unique_ptr<BCDatabase> db;

private slots:
    void onSelectionChanged();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
