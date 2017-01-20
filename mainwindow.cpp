#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QDateTime>

const QString MainWindow::storedState = QDir::homePath()+"/.squanthrough";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , progress(-1)
    , scanMotor(new QCaMotorGUI)
    , stepMotor(new QCaMotorGUI)
    , shut(new Shutter)
    , errorLabel(new QLabel)
{

    ui->setupUi(this);

    ColumnResizer * resizer;
    resizer = new ColumnResizer(this);
    ui->preExecScript->addToColumnResizer(resizer);
    ui->postExecScript->addToColumnResizer(resizer);
    ui->preScanScript->addToColumnResizer(resizer);
    ui->postScanScript->addToColumnResizer(resizer);
    ui->preStepScript->addToColumnResizer(resizer);
    ui->postStepScript->addToColumnResizer(resizer);

    ui->statusbar->addPermanentWidget(errorLabel);

    QPushButton * save = new QPushButton("Save");
    save->setFlat(true);
    connect(save, SIGNAL(clicked()), SLOT(saveConfiguration()));
    ui->statusbar->addPermanentWidget(save);

    QPushButton * load = new QPushButton("Load");
    load->setFlat(true);
    connect(load, SIGNAL(clicked()), SLOT(loadConfiguration()));
    ui->statusbar->addPermanentWidget(load);

    ui->startStop->setText("Start");

    ui->placeScanMotor->layout()->addWidget(scanMotor->setupButton());
    ui->placeStepMotor->layout()->addWidget(stepMotor->setupButton());

    foreach (Shutter::KnownShutters shut, Shutter::allKnownShutters)
      ui->shutterSelection->addItem(Shutter::shutterName(shut));
    //ui->shutterSelection->addItem("Custom");

    ui->stepPositionsList->setItemDelegate(new NTableDelegate(ui->stepPositionsList));

    loadConfiguration(storedState);

    connect(ui->browseExpPath, SIGNAL(clicked()), SLOT(onWorkingDirBrowse()));
    connect(ui->expPath, SIGNAL(editingFinished()), SLOT(onParameterChange()));

    connect( shut, SIGNAL(connectionUpdated()), SLOT(updateShutter()));
    connect( shut, SIGNAL(stateUpdated()), SLOT(updateShutter()));
    connect( ui->shutterSelection, SIGNAL(currentIndexChanged(int)), SLOT(onParameterChange()));

    connect( scanMotor->motor(), SIGNAL(changedUserPosition(double)), ui->scanCurrent, SLOT(setValue(double)) );
    connect( stepMotor->motor(), SIGNAL(changedUserPosition(double)), ui->stepCurrent, SLOT(setValue(double)) );
    connect( scanMotor->motor(), SIGNAL(changedPv(QString)), SLOT(onParameterChange()));
    connect( stepMotor->motor(), SIGNAL(changedPv(QString)), SLOT(onParameterChange()));
    connect( scanMotor->motor(), SIGNAL(changedUserPosition(double)), SLOT(onParameterChange()));
    connect( stepMotor->motor(), SIGNAL(changedUserPosition(double)), SLOT(onParameterChange()));
    connect( scanMotor->motor(), SIGNAL(changedMoving(bool)), SLOT(onParameterChange()));
    connect( stepMotor->motor(), SIGNAL(changedMoving(bool)), SLOT(onParameterChange()));
    connect( scanMotor->motor(), SIGNAL(changedConnected(bool)), SLOT(onParameterChange()));
    connect( stepMotor->motor(), SIGNAL(changedConnected(bool)), SLOT(onParameterChange()));

    connect( scanMotor->motor(), SIGNAL(changedMaximumSpeed(double)), ui->speedmax, SLOT(setValue(double)));
    connect( scanMotor->motor(), SIGNAL(changedNormalSpeed(double)), ui->speedcur, SLOT(setValue(double)));

    connect( ui->expPath, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->prefix, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->scanRepetitions, SIGNAL(valueChanged(int)), SLOT(onParameterChange()));
    connect( ui->scanDistance, SIGNAL(valueChanged(double)), SLOT(onParameterChange()));
    connect( ui->scanSpeed, SIGNAL(valueChanged(double)), SLOT(onParameterChange()));
    connect( ui->stepNof, SIGNAL(valueChanged(int)), SLOT(onParameterChange()));
    connect( ui->stepSize, SIGNAL(valueChanged(double)), SLOT(onParameterChange()));
    connect( ui->stepIrregular, SIGNAL(toggled(bool)), SLOT(onParameterChange()));
    connect( ui->stepPositionsList, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(onParameterChange()));

    connect( ui->preExecScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->postExecScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->preScanScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->postScanScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->preStepScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));
    connect( ui->postStepScript, SIGNAL(editingFinished()), SLOT(onParameterChange()));

    connect(ui->startStop, SIGNAL(clicked()), SLOT(onStartStop()));

    onParameterChange();
    updateProgress();

}



static void setInConfig(QSettings & config, const QString & key, QWidget * wdg) {
  if (key.isEmpty())
    return;
  if (dynamic_cast<QLineEdit*>(wdg))
    config.setValue(key, dynamic_cast<QLineEdit*>(wdg)->text());
  else if (dynamic_cast<QPlainTextEdit*>(wdg))
    config.setValue(key, dynamic_cast<QPlainTextEdit*>(wdg)->toPlainText());
  else if (dynamic_cast<QAbstractButton*>(wdg))
    config.setValue(key, dynamic_cast<QAbstractButton*>(wdg)->isChecked());
  else if (dynamic_cast<Script*>(wdg))
    config.setValue(key, dynamic_cast<Script*>(wdg)->path());
  else if (dynamic_cast<QComboBox*>(wdg))
    config.setValue(key, dynamic_cast<QComboBox*>(wdg)->currentText());
  else if (dynamic_cast<QSpinBox*>(wdg))
    config.setValue(key, dynamic_cast<QSpinBox*>(wdg)->value());
  else if (dynamic_cast<QDoubleSpinBox*>(wdg))
    config.setValue(key, dynamic_cast<QDoubleSpinBox*>(wdg)->value());
  else if (dynamic_cast<QCaMotorGUI*>(wdg))
    config.setValue(key, dynamic_cast<QCaMotorGUI*>(wdg)->motor()->getPv());
  else if ( dynamic_cast<QLabel*>(wdg))
    config.setValue(key, dynamic_cast<QLabel*>(wdg)->text());
  else
    qDebug() << "Cannot save the value of widget" << wdg << "into config";
}

static void restoreFromConfig(QSettings & config, const QString & key, QWidget * wdg) {
  if ( ! config.contains(key) ) {
    qDebug() << "Config does not contain key" << key;
    return;
  }
  QVariant value=config.value(key);
  if ( ! value.isValid() ) {
    qDebug() << "Value read from config key" << key << "is not valid";
    return;
  }

  if ( dynamic_cast<QLineEdit*>(wdg) && value.canConvert(QVariant::String) )
    dynamic_cast<QLineEdit*>(wdg)->setText(value.toString());
  else if ( dynamic_cast<QPlainTextEdit*>(wdg) && value.canConvert(QVariant::String) )
    dynamic_cast<QPlainTextEdit*>(wdg)->setPlainText(value.toString());
  else if ( dynamic_cast<QAbstractButton*>(wdg) && value.canConvert(QVariant::Bool) )
    dynamic_cast<QAbstractButton*>(wdg)->setChecked(value.toBool());
  else if ( dynamic_cast<Script*>(wdg) && value.canConvert(QVariant::String) )
    dynamic_cast<Script*>(wdg)->setPath(value.toString());
  else if ( dynamic_cast<QComboBox*>(wdg) && value.canConvert(QVariant::String) ) {
    const QString val = value.toString();
    QComboBox * box = dynamic_cast<QComboBox*>(wdg);
    const int idx = box->findText(val);
    if ( idx >= 0 )  box->setCurrentIndex(idx);
    else if ( box->isEditable() ) box->setEditText(val);
  }
  else if ( dynamic_cast<QSpinBox*>(wdg) && value.canConvert(QVariant::Int) )
    dynamic_cast<QSpinBox*>(wdg)->setValue(value.toInt());
  else if ( dynamic_cast<QDoubleSpinBox*>(wdg) && value.canConvert(QVariant::Double) )
    dynamic_cast<QDoubleSpinBox*>(wdg)->setValue(value.toDouble());
  else if ( dynamic_cast<QCaMotorGUI*>(wdg) && value.canConvert(QVariant::String) )
    dynamic_cast<QCaMotorGUI*>(wdg)->motor()->setPv(value.toString());
  else
    qDebug() << "Cannot restore the value of widget" << wdg
             << "from value" << value << "in config.";
}


void MainWindow::saveConfiguration(QString fileName) {

  if ( fileName.isEmpty() )
    fileName = QFileDialog::getSaveFileName(0, "Save configuration", QDir::currentPath());

  QSettings config(fileName, QSettings::IniFormat);

  setInConfig(config, "prefix", ui->prefix);
  setInConfig(config, "path", ui->expPath);
  setInConfig(config, "shutter", ui->shutterSelection);
  setInConfig(config, "customShutter", ui->shutterSelection);
  setInConfig(config, "scanmotor", scanMotor);
  setInConfig(config, "scanrepititions", ui->scanRepetitions);
  setInConfig(config, "scandistance", ui->scanDistance);
  setInConfig(config, "scanspeed", ui->scanSpeed);
  setInConfig(config, "stepmotor", stepMotor);
  setInConfig(config, "steps", ui->stepNof);
  setInConfig(config, "stepsize", ui->stepSize);
  setInConfig(config, "stepirregular", ui->stepIrregular);
  if ( ui->stepIrregular->isChecked() ) {
    config.beginWriteArray("steplist");
    int index = 0;
    foreach (QTableWidgetItem * item,
             ui->stepPositionsList->findItems("", Qt::MatchContains) ) {
      config.setArrayIndex(index++);
      config.setValue("position", item ? item->text() : "");
    }
    config.endArray();
  }
  setInConfig(config, "scripting", ui->scripting);
  setInConfig(config, "preexecscript",  ui->preExecScript);
  setInConfig(config, "postexecscript",  ui->postExecScript);
  setInConfig(config, "prescanscript",  ui->preScanScript);
  setInConfig(config, "postscanscript",  ui->postScanScript);
  setInConfig(config, "prestepscript",  ui->preStepScript);
  setInConfig(config, "poststepscript",  ui->postStepScript);

}


void MainWindow::loadConfiguration(QString fileName) {

  if ( fileName.isEmpty() )
    fileName = QFileDialog::getOpenFileName(0, "Load configuration", QDir::currentPath());
  if ( ! QFile::exists(fileName) ) {
    qDebug() << "Configuration file \"" + fileName + "\" does not exist.";
    return;
  }
  QSettings config(fileName, QSettings::IniFormat);

  restoreFromConfig(config, "prefix", ui->prefix);
  restoreFromConfig(config, "path", ui->expPath);
  restoreFromConfig(config, "shutter", ui->shutterSelection);
  restoreFromConfig(config, "scanmotor", scanMotor);
  restoreFromConfig(config, "scanrepititions", ui->scanRepetitions);
  restoreFromConfig(config, "scandistance", ui->scanDistance);
  restoreFromConfig(config, "scanspeed", ui->scanSpeed);
  restoreFromConfig(config, "stepmotor", stepMotor);
  restoreFromConfig(config, "steps", ui->stepNof);
  restoreFromConfig(config, "stepsize", ui->stepSize);
  restoreFromConfig(config, "stepirregular", ui->stepIrregular);
  if ( ui->stepIrregular->isChecked() ) {
    int stepssize = config.beginReadArray("steplist");
    for (int i = 0; i < stepssize; ++i) {
      config.setArrayIndex(i);
      if ( ui->stepPositionsList->item(i, 0) )
        ui->stepPositionsList->item(i,0)->setText
            (config.value("position").toString());
    }
    config.endArray();
  }
  restoreFromConfig(config, "scripting", ui->scripting);
  restoreFromConfig(config, "preexecscript",  ui->preExecScript);
  restoreFromConfig(config, "postexecscript",  ui->postExecScript);
  restoreFromConfig(config, "prescanscript",  ui->preScanScript);
  restoreFromConfig(config, "postscanscript",  ui->postScanScript);
  restoreFromConfig(config, "prestepscript",  ui->preStepScript);
  restoreFromConfig(config, "poststepscript",  ui->postStepScript);

}



MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::updateProgress() {
    ui->mainWidget->setEnabled(progress<0);
    ui->progressBar->setVisible(progress>=0);
    ui->progressBar->setValue(progress);
    ui->startStop->setText( progress<0 ? "Start" : "Interrupt" );
}

void MainWindow::onWorkingDirBrowse() {
    ui->expPath->setText( QFileDialog::getExistingDirectory(0, "Working directory", QDir::currentPath()) );
}

void MainWindow::onParameterChange() {

    if (progress >= 0)
        return;

    saveConfiguration(storedState);

    check(ui->expPath, QDir(ui->expPath->text()).exists() );

    const double scanend = scanMotor->motor()->getUserPosition() + ui->scanDistance->value();
    check(ui->scanDistance,
          ui->scanDistance->value() != 0.0  &&
          scanend > scanMotor->motor()->getUserLoLimit() &&
          scanend < scanMotor->motor()->getUserHiLimit() );
    check(scanMotor->setupButton(), scanMotor->motor()->isConnected()  &&  ! scanMotor->motor()->isMoving() );
    check(ui->scanCurrent, scanMotor->motor()->isConnected());
    check(ui->scanSpeed, ui->scanSpeed->value() > 0  && ui->scanSpeed->value() <= scanMotor->motor()->getMaximumSpeed() );

    const QString scanUnits = scanMotor->motor()->isConnected() ? scanMotor->motor()->getUnits() : QString();
    ui->scanCurrent->setSuffix(scanUnits);
    ui->scanDistance->setSuffix(scanUnits);
    ui->scanSpeed->setSuffix(scanUnits+"/s");
    ui->speedcur->setSuffix(" " + scanUnits+"/s");
    ui->speedmax->setSuffix(" " + scanUnits+"/s");


    ui->stepSize->setEnabled( ! ui->stepIrregular->isChecked() );
    check(stepMotor->setupButton(),
          stepMotor->motor()->isConnected()  &&  ! stepMotor->motor()->isMoving());

    while ( ui->stepNof->value() < ui->stepPositionsList->rowCount() )
        ui->stepPositionsList->removeRow( ui->stepPositionsList->rowCount()-1 );
    while ( ui->stepNof->value() > ui->stepPositionsList->rowCount() ) {
      ui->stepPositionsList->insertRow(ui->stepPositionsList->rowCount());
      ui->stepPositionsList->setItem( ui->stepPositionsList->rowCount()-1, 0,
        new QTableWidgetItem( QString::number(stepMotor->motor()->getUserPosition() ) ) );
    }

    bool allOK=true;
    foreach (QTableWidgetItem * item,
             ui->stepPositionsList->findItems("", Qt::MatchContains) ) {

        ui->stepPositionsList->blockSignals(true);
        if ( ui->stepIrregular->isChecked() )
            item->setFlags( item->flags() | Qt::ItemIsEditable );
        else
            item->setFlags( item->flags() & ~Qt::ItemIsEditable );
        ui->stepPositionsList->blockSignals(false);

        if ( ! ui->stepIrregular->isChecked()  &&  ! stepMotor->motor()->isMoving() ) {
            double pos = stepMotor->motor()->getUserPosition() + item->row() * ui->stepSize->value();
            if ( pos != item->text().toDouble() )
                item->setText( QString::number(pos) );
        }

        bool isDouble;
        const double pos = item->text().toDouble(&isDouble);
        const bool isOK = isDouble &&
                (pos > stepMotor->motor()->getUserLoLimit() ) &&
                (pos < stepMotor->motor()->getUserHiLimit() );
        item->setBackground( isOK ? QBrush() : QBrush(QColor(Qt::red)));
        allOK &= isOK;

    }

    check( ui->stepIrregular,  ! ui->stepIrregular->isChecked() || allOK );

    const QString stepUnits = stepMotor->motor()->isConnected() ? stepMotor->motor()->getUnits() : QString();
    ui->stepCurrent->setSuffix(stepUnits);
    ui->stepSize->setSuffix(stepUnits);

    ui->progressBar->setMaximum( ui->scanRepetitions->value() * ui->stepNof->value() );

    ui->customShutter->setVisible( ui->shutterSelection->currentText() == "Custom" );
    if ( ui->shutterSelection->currentText() != shut->shutterName() )
      shut->setShutter( Shutter::shutterName( ui->shutterSelection->currentText() ) );

    setenv("SQUANTHROUGH_LOG", ".tmp.log", 1);
    setenv("SQUANTHROUGH_STEPS", ui->stepNof->text().toAscii(), 1);
    setenv("SQUANTHROUGH_REPTS", ui->scanRepetitions->text().toAscii(), 1);

    updateProgress();

}


void MainWindow::updateShutter() {
  const bool avail=shut->isAvailable();
  ui->shutterToggle->setEnabled(avail);
  check(ui->shutterStatus, avail);
  if ( ! shut->isAvailable() ) {
    ui->shutterStatus->setText("Inavailable");
    ui->shutterToggle->setText("Toggle");
  } else if ( shut->isOpened() ) {
    ui->shutterStatus->setText("Open");
    ui->shutterToggle->setText("Close");
  } else {
    ui->shutterStatus->setText("Closed");
    ui->shutterToggle->setText("Open");
  }
}



void MainWindow::check(QWidget * obj, bool status) {

  if ( progress>=0 || ! obj )
    return;

  preReq[obj] = status;
  obj->setStyleSheet( status  ?  ""  :  "background-color: rgba(255, 0, 0, 128);" );
  bool allOK=status;
  foreach(bool cur, preReq)
      allOK &= cur;
  ui->startStop->setEnabled( allOK || progress >= 0 );

}


static void logToFile(const QString & file, const QString & message) {
  QFile logfl(file);
  const QString lntoappend = QDateTime::currentDateTime().toString(Qt::ISODate) + " " + message + "\n";
  if ( ! logfl.open(QIODevice::Append)  ||
       logfl.write( lntoappend.toAscii() ) == -1 ) {
    qDebug() << "Failed to write into log file \"" << file << "\". Lost message : \"" << lntoappend << "\"";
    return;
  }
  logfl.close();
}


void MainWindow::onStartStop() {

  static QString logName;

  if (progress>=0) {
    logToFile(logName, "interrupting experiment");
    progress = -1;
    updateProgress();
    return;
  }

  const int
      nofRepetitions = ui->scanRepetitions->value(),
      nofSteps = ui->stepNof->value();
  const float
      scanDist = ui->scanDistance->value(),
      scanSpeed = ui->scanSpeed->value(),
      initSpeed = scanMotor->motor()->getNormalSpeed(),
      initScanPosition = scanMotor->motor()->getUserPosition(),
      initStepPosition = stepMotor->motor()->getUserPosition();
  static const QString
      shutError("ERROR! Shut failed to "),
      limitError("ERROR! Unexpected motor limit activated: "),
      errorSS("color: rgba(255, 0, 0, 128);")
      ;
  errorLabel->setStyleSheet("");
  errorLabel->setText("");


  ui->progressBar->setMaximum(nofRepetitions*nofSteps);

  // config and log
  QString prefix = ui->prefix->text();
  prefix = (prefix.isEmpty() ? "squanthrough" : prefix) + "."
      +  QDateTime::currentDateTime().toString(Qt::ISODate);
  const QString
      cfgName = prefix + ".cfg";
  logName = prefix + ".log";

  progress = 0;
  updateProgress();


  saveConfiguration(cfgName);
  setenv("SQUANTHROUGH_LOG", logName.toAscii(), 1);
  logToFile(logName, "sarting experiment");
  if (progress < 0) goto onExperimentCompletion;


  setenv("SQUANTHROUGH_STEPS", QString::number(nofSteps).toAscii(), 1);
  setenv("SQUANTHROUGH_REPTS", QString::number(nofRepetitions).toAscii(), 1);
  if ( ui->preExecScript->execute() )
    logToFile(logName, "Warning! Pre-execution script returned with non-zero exit status.");

  if (progress < 0) goto onExperimentCompletion;

  if ( stepMotor->motor()->isConnected() &&
       ui->stepIrregular->isChecked() ) // otherwise is already in the first point
    stepMotor->motor()->goUserPosition(
          ui->stepPositionsList->item(0, 0)->text().toDouble(), QCaMotor::STARTED);
  if (progress < 0) goto onExperimentCompletion;

  logToFile(logName, "starting scans");



  for (int curStep=0 ; curStep < nofSteps ; curStep++) {

    if (stepMotor->motor()->isConnected()) {
      stepMotor->motor()->wait_stop();
      if (stepMotor->motor()->getLimitStatus())  {
        logToFile(logName, limitError + "step motor @ " + QString::number(stepMotor->motor()->getUserPosition()));
        errorLabel->setStyleSheet(errorSS);
        errorLabel->setText(limitError + "step motor");
      }
    }

    setenv("SQUANTHROUGH_STEP", QString::number(curStep).toAscii(), 1);
    if ( ui->preStepScript->execute() )
      logToFile(logName, "Warning! Pre-step script returned with non-zero exit status.");
    if (progress < 0) goto onExperimentCompletion;

    logToFile(logName, "starting step " + QString::number(curStep) );



    for (int curRep=0 ; curRep < nofRepetitions ; curRep++) {

      scanMotor->motor()->wait_stop();
      if (scanMotor->motor()->getLimitStatus())  {
        logToFile(logName, limitError + "scan motor @ " + QString::number(scanMotor->motor()->getUserPosition()));
        errorLabel->setStyleSheet(errorSS);
        errorLabel->setText(limitError + "scan motor");
      }
      if (scanMotor->motor()->getNormalSpeed() != scanSpeed) {
        scanMotor->motor()->setNormalSpeed(scanSpeed);
        scanMotor->motor()->waitUpdated<double>
            (".VELO", scanSpeed, &QCaMotor::getNormalSpeed, 500);
      }
      if (progress < 0) goto onExperimentCompletion;

      setenv("SQUANTHROUGH_REPT", QString::number(curRep).toAscii(), 1);
      if ( ui->preScanScript->execute() )
        logToFile(logName, "Warning! Pre-scan script returned with non-zero exit status.");
      if (progress < 0) goto onExperimentCompletion;

      logToFile(logName, "starting repetition " + QString::number(curRep) + " of step " + QString::number(curStep) );
      shut->open(true);
      if ( ! shut->isOpened() ) {
        logToFile(logName, shutError + "open");
        errorLabel->setStyleSheet(errorSS);
        errorLabel->setText(shutError + "open");
      }
      if (progress < 0) goto onExperimentCompletion;
      scanMotor->motor()->goRelative(scanDist, QCaMotor::STARTED);
      if (progress < 0) goto onExperimentCompletion;
      scanMotor->motor()->wait_stop();
      if (progress < 0) goto onExperimentCompletion;
      shut->close(true);

      if (scanMotor->motor()->getLimitStatus())  {
        logToFile(logName, limitError + "scan motor @ " + QString::number(scanMotor->motor()->getUserPosition()));
        errorLabel->setStyleSheet(errorSS);
        errorLabel->setText(limitError + "scan motor");
      }
      if ( shut->isOpened() ) {
        logToFile(logName, shutError + "close");
        errorLabel->setStyleSheet(errorSS);
        errorLabel->setText(shutError + "close");
      }
      if (progress < 0) goto onExperimentCompletion;

      logToFile(logName, "finished repetition " + QString::number(curRep) + " of step " + QString::number(curStep) );

      if ( ui->postScanScript->execute() )
        logToFile(logName, "Warning! Post-scan script returned with non-zero exit status.");
      if (progress < 0) goto onExperimentCompletion;

      if (scanMotor->motor()->getNormalSpeed() != initSpeed) {
        scanMotor->motor()->setNormalSpeed(initSpeed);
        scanMotor->motor()->waitUpdated<double>
            (".VELO", initSpeed, &QCaMotor::getNormalSpeed, 500);
      }
      scanMotor->motor()->goUserPosition(initScanPosition, QCaMotor::STARTED);
      if (progress < 0) goto onExperimentCompletion;

      ui->progressBar->setValue(++progress);

    }

    logToFile(logName, "finished step " + QString::number(curStep) );

    if ( ui->postStepScript->execute() )
      logToFile(logName, "Warning! Post-step script returned with non-zero exit status.");
    if (progress < 0) goto onExperimentCompletion;

    if ( stepMotor->motor()->isConnected() &&
         ui->stepPositionsList->rowCount() > curStep+1 )
      stepMotor->motor()->goUserPosition(
            ui->stepPositionsList->item(curStep+1, 0)->text().toDouble(), QCaMotor::STARTED);
    if (progress < 0) goto onExperimentCompletion;

  }

  logToFile(logName, "finished all scans");

  if ( ui->postExecScript->execute() )
    logToFile(logName, "Warning! Post-execution script returned with non-zero exit status.");

onExperimentCompletion :


  ui->progressBar->setValue(++progress);

  shut->close(false);

  if ( stepMotor->motor()->isConnected() ) {
    stepMotor->motor()->stop(QCaMotor::STOPPED);
    stepMotor->motor()->goUserPosition(initStepPosition, QCaMotor::STARTED);
  }
  scanMotor->motor()->stop(QCaMotor::STOPPED);
  if (scanMotor->motor()->getNormalSpeed() != initSpeed) {
    scanMotor->motor()->setNormalSpeed(initSpeed);
    scanMotor->motor()->waitUpdated<double>
        (".VELO", initSpeed, &QCaMotor::getNormalSpeed, 500);
  }
  scanMotor->motor()->goUserPosition(initScanPosition, QCaMotor::STARTED);

  logToFile(logName, "finished experiment");

  progress = -1;
  updateProgress();

  return;

}










