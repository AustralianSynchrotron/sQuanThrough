#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qcamotorgui.h>
#include <additional_classes.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private:

    Ui::MainWindow *ui;
    int progress;
    static const QString storedState;
    QCaMotorGUI * scanMotor;
    QCaMotorGUI * stepMotor;
    Shutter * shut;

    QHash <const QWidget*,  bool > preReq;
    void check(QWidget * obj, bool status);

private slots:

    void updateProgress();
    void updateShutter();
    void onWorkingDirBrowse();
    void onParameterChange();
    void onStartStop();

    void loadConfiguration( QString fileName = QString() );
    void saveConfiguration( QString fileName = QString() );

};



#endif // MAINWINDOW_H
