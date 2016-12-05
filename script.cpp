#include "script.h"
#include "ui_script.h"

Script::Script(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Script),
  fileExec(this)
{
  ui->setupUi(this);
  connect(ui->path, SIGNAL(textChanged(QString)), SLOT(evaluate()));
  connect(ui->browse, SIGNAL(clicked()), SLOT(browse()));
  connect(ui->execute, SIGNAL(clicked()), SLOT(onStartStop()));
  connect(ui->path, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
  connect(&proc, SIGNAL(stateChanged(QProcess::ProcessState)),
          SLOT(onState(QProcess::ProcessState)));
  if ( ! fileExec.open() )
    qDebug() << "ERROR! Unable to open temporary file.";
}


Script::~Script() {
  delete ui;
}

void Script::setPath(const QString & _path) {
  ui->path->setText(_path);
}

const QString Script::path() const {
  return ui->path->text();
}

void Script::browse() {
  ui->path->setText(
        QFileDialog::getOpenFileName(0, "Command", QDir::currentPath()) );
}


void Script::evaluate() {

  if ( ! fileExec.isOpen() || isRunning() )
    return;
  ui->execute->setStyleSheet("");

  fileExec.resize(0);
  fileExec.write( ui->path->text().toAscii() );
  fileExec.flush();

  QProcess tempproc;
  tempproc.start("/bin/sh -n " + fileExec.fileName());
  tempproc.waitForFinished();
  ui->path->setStyleSheet( tempproc.exitCode() ? "color: rgb(255, 0, 0);" : "");

}


bool Script::start() {
  if ( ! fileExec.isOpen() || isRunning() )
    return false;
  if (ui->path->text().isEmpty())
    return true;
  proc.start("/bin/sh " + fileExec.fileName());
  return isRunning();
}

int Script::waitStop() {
  QEventLoop q;
  connect(&proc, SIGNAL(finished(int)), &q, SLOT(quit()));
  if (isRunning())
    q.exec();
  return proc.exitCode();
}

void Script::onState(QProcess::ProcessState state) {

  ui->browse->setEnabled( state==QProcess::NotRunning );
  ui->path->setEnabled( state==QProcess::NotRunning );
  ui->execute->setText( state==QProcess::NotRunning ?
                         "Execute" : "Stop" );

  if (state==QProcess::NotRunning) {
    ui->execute->setStyleSheet( proc.exitCode() ? "color: rgb(255, 0, 0);" : "");
    emit finished(proc.exitCode());
  } else if (state==QProcess::Running) {
    emit started();
  }

};


void Script::addToColumnResizer(ColumnResizer * columnizer) {
  if ( ! columnizer )
    return;
  columnizer->addWidgetsFromGridLayout(ui->gridLayout, 1);
  columnizer->addWidgetsFromGridLayout(ui->gridLayout, 2);
}


