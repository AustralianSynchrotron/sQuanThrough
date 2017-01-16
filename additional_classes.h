#include <QPlainTextEdit>
#include <QLabel>
#include <QDebug>
#include <QLineEdit>
#include <QToolButton>
#include <QStyle>
#include <QProcess>
#include <QTemporaryFile>
#include <QGridLayout>
#include <QTableWidget>
#include <QWidgetList>
#include <QProgressBar>
#include <QCheckBox>
#include <QStyledItemDelegate>
#include <qtpv.h>

#ifndef CTGUIADDITIONALCLASSES
#define CTGUIADDITIONALCLASSES

/*
namespace Ui {
  class QSCheckBox;
}
*/

class QSCheckBox : public QCheckBox {
  Q_OBJECT;
  //Ui::QSCheckBox ui;
public :
  QSCheckBox(QWidget * parent = 0);

protected:
  virtual bool hitButton ( const QPoint & pos ) const ;

};



struct ColumnResizerPrivate;
class ColumnResizer : public QObject {
  Q_OBJECT;
public:
  ColumnResizer(QObject* parent = 0);
  ~ColumnResizer();

  void addWidget(QWidget* widget);
  void addWidgetsFromGridLayout(QGridLayout*, int column);

private Q_SLOTS:
  void updateWidth();

protected:
  bool eventFilter(QObject*, QEvent* event);

private:
  ColumnResizerPrivate* const d;
};



class QPTEext : public QPlainTextEdit {
  Q_OBJECT;
public:
  QPTEext(QWidget * parent=0) : QPlainTextEdit(parent) {};
signals:
  void editingFinished();
  void focusIned();
  void focusOuted();
protected:
  bool event(QEvent *event);
  void focusInEvent ( QFocusEvent * e );
  void focusOutEvent ( QFocusEvent * e );
};


class CtGuiLineEdit : public QLineEdit {
  Q_OBJECT;
private:
  QToolButton *clearButton;
public:
  CtGuiLineEdit(QWidget *parent);
protected:
  void resizeEvent(QResizeEvent *);
private slots:
  void updateCloseButton(const QString& text) {
    clearButton->setVisible(!text.isEmpty());
  }
};



class EasyTabWidget : public QTabWidget {
  Q_OBJECT;
  QWidgetList wdgs;
  QHash<QWidget*, QString> titles;

public:
  EasyTabWidget( QWidget * parent = 0 ) : QTabWidget(parent) {}
  void finilize();
  void setTabVisible(QWidget * tab, bool visible);
  void setTabTextColor(QWidget * tab, const QColor & color);
  const QWidgetList & tabs() const {return wdgs;};
  QWidgetList tabs() {return wdgs;}

};



class CTprogressBar : public QProgressBar {
  Q_OBJECT;
  QLabel * label;
  void updateLabel();

public:
  CTprogressBar(QWidget * parent=0);
  void setTextVisible(bool visible) ;

public slots:
  void setValue(int value) ;
  void setMinimum(int minimum);
  void setMaximum(int maximum);

protected:
  virtual void	resizeEvent(QResizeEvent * event);

};



namespace Ui {
class Script;
}

class Script : public QWidget {
  Q_OBJECT;

private:
  Ui::Script *ui;
  QProcess proc;
  QTemporaryFile fileExec;

public:
  explicit Script(QWidget *parent = 0);
  ~Script();

  void setPath(const QString & _path);
  const QString out() {return proc.readAllStandardOutput();}
  const QString err() {return proc.readAllStandardError();}
  int waitStop();
  bool isRunning() const { return proc.pid(); };
  const QString path() const;

  void addToColumnResizer(ColumnResizer * columnizer);

public slots:
  bool start();
  int execute() { return start() ? waitStop() : -1 ; };
  void stop() {if (isRunning()) proc.kill();};

private slots:
  void browse();
  void evaluate();
  void onState(QProcess::ProcessState state);
  void onStartStop() { if (isRunning()) stop(); else start(); };
  void updateShutter();

signals:
  void editingFinished();
  void executed();
  void finished(int status);
  void started();

};


class MotorPosition : public QWidget {
  Q_OBJECT;

public:

  explicit MotorPosition(QWidget *parent = 0);


};



// to be used for the doble numbers in the list of positions
// in the irregular step serial scans.

class NTableDelegate : public QStyledItemDelegate {
  Q_OBJECT;
public:
  NTableDelegate(QObject* parent);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

// QTableWidget with support for copy and paste added
// Here copy and paste can copy/paste the entire grid of cells
class QTableWidgetWithCopyPaste : public QTableWidget
{
public:
  QTableWidgetWithCopyPaste(int rows, int columns, QWidget *parent) :
      QTableWidget(rows, columns, parent)
  {};

  QTableWidgetWithCopyPaste(QWidget *parent) :
    QTableWidget(parent)
  {};

private:
  void copy();
  void paste();

protected:
  virtual void keyPressEvent(QKeyEvent * event);
};



class Shutter : public QObject {

  Q_OBJECT;
  enum KnownShutters {
    CUSTOM=0,
    Beamline,
    MRT,
    Imaging
  };
  static QString shutterName(KnownShutters shut) {
    switch (shut) {
      case Beamline: return "Beamline";
      case MRT:      return "MRT in 2A";
      case Imaging:  return "Imaging";
      default:       return "";
    }
  }

  static KnownShutters shutterName(const QString & shut) {
    if (shut == "Beamline")  return Beamline;
    if (shut == "MRT in 2A") return MRT;
    if (shut == "Imaging")   return Imaging;
    return CUSTOM;
  }

  KnownShutters inCharge;

  QEpicsPv * openPv;
  QEpicsPv * closePv;
  QEpicsPv * openStatPv;
  QEpicsPv * closedStatPv;

  QHash < QEpicsPv ** , QVariant > pvs;

public:

  Shutter(QObject * parent = 0, KnownShutters inCh = KnownShutters::CUSTOM )
    : QObject(parent)
    , inCharge(inCh)
    , openPv(0)
    , closePv(0)
    , openStatPv(0)
    , closedStatPv(0)
  {
    setShutter(inCh);
  }

  bool isOpened() { return isAvailable() &&
                           openStatPv->get().toString() == pvs[&openStatPv] ; }

  bool isAvailable() { return
        openPv && openPv->isConnected() &&
        closePv && closePv->isConnected() &&
        openStatPv && openStatPv->isConnected() &&
        closedStatPv && closedStatPv->isConnected() ; }

public slots:

  void setShutter(KnownShutters shut) {

    switch (shut) {
      case Beamline:
        setShutter( QList <QPair <QString, QVariant> > ()
                    << QPair<QString,QVariant>("SR08ID01PSS01:HU01A_BL_SHUTTER_OPEN_CMD", 1)
                    << QPair<QString,QVariant>("SR08ID01PSS01:HU01A_BL_SHUTTER_CLOSE_CMD", 1)
                    << QPair<QString,QVariant>("SR08ID01PSS01:HU01A_SF_SHUTTER_OPEN_STS", 1)
                    << QPair<QString,QVariant>("SR08ID01PSS01:HU01A_SF_SHUTTER_CLOSE_STS", 1)  );
        break;
      case MRT:
        setShutter( QList <QPair <QString, QVariant> > ()
                    << QPair<QString,QVariant>("SR08ID01MRT01:SHUTTEROPEN_CMD", "Open")
                    << QPair<QString,QVariant>("", "Close")
                    << QPair<QString,QVariant>("SR08ID01MRT01:SHUTTEROPEN_MONITOR", "Opened")
                    << QPair<QString,QVariant>("", "Closed") );
        break;
      case Imaging:
        setShutter( QList <QPair <QString, QVariant> > ()
                    << QPair<QString,QVariant>("SR08ID01IS01:SHUTTEROPEN_CMD", "Open")
                    << QPair<QString,QVariant>("", "Close")
                    << QPair<QString,QVariant>("SR08ID01IS01:SHUTTEROPEN_MONITOR", "Opened")
                    << QPair<QString,QVariant>("", "Closed")  );
      default:
        pvs.clear();
        break;
    }

    inCharge=shut;

  }

  void setShutter( const QList <QPair <QString, QVariant> > & pvlst ) {

    pvs.clear();
    inCharge=CUSTOM;

    if (pvlst.size() != 4) {
      qDebug() << "Unknown format for the custom shutter.";
      return;
    }


    if (openPv) delete openPv;
    openPv = new QEpicsPv(pvlst[0].first, this);
    pvs[&openPv]=pvlst[0].second;
    connect(openPv, SIGNAL(connectionUpdated(bool)), SIGNAL(connectionUpdated()));

    if (closePv) delete closePv;
    closePv = pvlst[1].first.isEmpty() || pvlst[1].first == pvlst[0].first
        ? openPv : new QEpicsPv(pvlst[1].first, this);
    pvs[&closePv]=pvlst[1].second;
    connect(closePv, SIGNAL(connectionUpdated(bool)), SIGNAL(connectionUpdated()));

    if (openStatPv) delete openStatPv;
    openStatPv = new QEpicsPv(pvlst[2].first, this);
    pvs[&openStatPv]=pvlst[2].second;
    connect(openStatPv, SIGNAL(connectionUpdated(bool)), SIGNAL(connectionUpdated()));
    connect(openStatPv, SIGNAL(valueUpdated(QVariant)), SIGNAL(stateUpdated()));
    connect(openStatPv, SIGNAL(valueUpdated(QVariant)), SIGNAL(stateUpdated()));

    if (closedStatPv) delete closedStatPv;
    closedStatPv = pvlst[3].first.isEmpty() || pvlst[3].first == pvlst[2].first
        ? openStatPv : new QEpicsPv(pvlst[3].first, this);
    pvs[&closedStatPv]=pvlst[3].second;
    connect(closedStatPv, SIGNAL(connectionUpdated(bool)), SIGNAL(connectionUpdated()));
    connect(closedStatPv, SIGNAL(valueUpdated(QVariant)), SIGNAL(stateUpdated()));

  }

  void open(bool waitcompletion=true) {
    if (!isAvailable())
      return;
    openPv->set(pvs[&openPv]);
    if (waitcompletion && ! isOpened() )
      qtWait(openStatPv, SIGNAL(valueUpdated(QVariant)), 2000);
  }

  void close(bool waitcompletion=true) {
    if (!isAvailable())
      return;
    closePv->set(pvs[&closePv]);
    if (waitcompletion && isOpened() )
      qtWait(closedStatPv, SIGNAL(valueUpdated(QVariant)), 2000);
  }

signals:

  void stateUpdated();
  openStatUpdated(QVariant);
  closedStatUpdated(QVariant);
  void connectionUpdated();


};


#endif // CTGUIADDITIONALCLASSES
