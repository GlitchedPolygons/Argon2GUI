#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_clearButton_clicked();

    void on_hashButton_clicked();

    void on_timeCostHorizontalSlider_valueChanged(int value);

    void on_memoryCostHorizontalSlider_valueChanged(int value);

    void on_parallelismHorizontalSlider_valueChanged(int value);

    void on_hashLengthHorizontalSlider_valueChanged(int value);

    void onChangedHashAlgorithm(int id);

    void on_encodedHashTextEdit_selectionChanged();

private:
    Ui::MainWindow *ui;
    bool busy = false;
    QString userEntropy;

    void appendEntropy(const QString& entropy);
    const char* getHashFunctionName() const;
};
#endif // MAINWINDOW_H
