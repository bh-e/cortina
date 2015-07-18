/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.
        
                    <Copyright (C) 2010-2012 Eric Baudach>
 */

#include "widget.h"
#include "ui_widget.h"
#include "helper.h"
#include <QNetworkProxyFactory>
//#include <libnotify/notify.h>

void MessageOutput(QtMsgType type, const char *msg)
{
    QString msg2;
    switch (type) {
    case QtDebugMsg:
#ifdef QT_DEBUG
        fprintf(stderr, "Debug: %s\n", msg);
#endif
        break;
    case QtWarningMsg:
        if(QString(msg).contains("null image")) {
            qDebug() << "Couldn't load image!";
            return;
        }
        if(QString(msg).contains("Application asked to unregister timer")) {
            qDebug() << "Application asked to unregister timer, which is not registered in this thread.";
            return;
        }
        fprintf(stderr, "Warning: %s\n", msg);
        //QMessageBox::warning(NULL, "Warning", QString(msg).remove('"'));
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        msg2 = QString(msg) + "\n\nPlease report the bug at:\nhttps://bugs.launchpad.net/cortina/+filebug";
        QMessageBox::critical(NULL, "Critical", QString(msg2).remove('"'));
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        msg2 = QString(msg) + "\n\nPlease report the bug at:\nhttps://bugs.launchpad.net/cortina/+filebug";
        QMessageBox::critical(NULL, "Fatal", QString(msg2).remove('"'));
        abort();
        break;
    }
}

Widget::Widget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Widget)
{
    MessageHandler = qInstallMsgHandler(MessageOutput);
    ui->setupUi(this);
    this->current = 0;
    this->firstload = 0;
    this->firstTime = false;
    this->WidgetIcon = false;
    this->ImageSavedWatcher = false;
    helper *thumb_del_thread = new helper();
    //    this->thumb_del_thread->autoDelete();
    qRegisterMetaType<QStringList>("QStringList");
    QObject::connect (this,
                      SIGNAL(helper_remove_file_thumbs_signal (const QStringList& )),
                      thumb_del_thread,
                      SLOT(Remove_File_thumb (const QStringList& )));
    
    QObject::connect (this,
                      SIGNAL(helper_remove_dir_thumbs_signal (const int&, const QStringList& )),
                      thumb_del_thread,
                      SLOT(Remove_Dir_thumb (const int&, const QStringList& )));
    thumb_del_thread->start();
    Tray = new QSystemTrayIcon(QIcon(QString::fromUtf8(":/icons/cortina.svg")), this);
    TrayContextMenu = new QMenu();
    TrayContextMenu->addAction(trUtf8("Open GUI"), this, SLOT(StartGui()), 0);
    const QString removeFromDiskString = QString(trUtf8("remove from disk"));
    removeAction = new QAction(removeFromDiskString, this);
    connect(removeAction, SIGNAL(triggered()), this, SLOT(DeleteCurrentWP()));
    QMenu *CurrentWallpaper = new QMenu(trUtf8("Current wallpaper"));
    CurrentWallpaper->addAction(removeAction);
    
    if(!QFileInfo(this->currentWP()).isWritable()) {
        removeAction->setEnabled(false);
    }
    TrayContextMenu->addMenu(CurrentWallpaper);
    TrayContextMenu->addSeparator();
    TrayContextMenu->addAction(trUtf8("Exit"), this, SLOT(close()), 0);
    this->Tray->setContextMenu(TrayContextMenu);
    setTooltipText();
    Tray->show();
    
    if(!connect(this->Tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(TrayClick(QSystemTrayIcon::ActivationReason)))) {
        qDebug() << "Tray Connect Failure!";
    }
    connect(ui->listWidget_Dirs, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemCheckStateChanged(QListWidgetItem*)));
    connect(ui->comboBox_wpstyle, SIGNAL(currentIndexChanged(int)), this, SLOT(SaveWpStyle(int)));
    Settings = new QSettings(QSettings::NativeFormat,QSettings::UserScope,QString("Cortina"),QString("Cortina"), NULL);
    b = new threadb();
    connect(b, SIGNAL(ImageSaved(bool)), this, SLOT(ImageSaved(bool)));
    connect(thumb_del_thread, SIGNAL(ImageSaved(bool)), this, SLOT(ImageSaved(bool)));
    connect(thumb_del_thread, SIGNAL(RemoveFromUI_signal(QByteArray)), this, SLOT(RemoveFromUI(QByteArray)));
    
    connect(this, SIGNAL(ThreadShouldSleep(int)), this->b, SLOT(sleeping(int)), Qt::AutoConnection);
    this->CreateUI();
    watcher = new QFileSystemWatcher();
    watcher->connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(DirChanged()));
    this->LoadSettings();
    this->ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(hider()));
    if(!connect(ui->listWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(listWidget_doubleClicked(QModelIndex)))) {
        qDebug() << "listWidget Connect Failure!";
    }
    if(!connect(ui->listWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(listWidget_customContextMenuRequested(const QPoint &)))) {
        qDebug() << "listWidget Connect Failure!";
    }
    if(!connect(ui->checkBox_loadOnStartup, SIGNAL(toggled(bool)), this, SLOT(setAutoStart(bool)))) {
        qDebug() << "checkBox_loadOnStartup Connect Failure!";
    }
    connect(this, SIGNAL(toAddSignal(const QStringList &)), b, SLOT(FileAdded(const QStringList &)), Qt::AutoConnection);
    connect(this, SIGNAL(shown(const QStringList &)), b, SLOT(FileAdded(const QStringList &)));
    connect(this, SIGNAL(toRemSignal(const QStringList &)), this, SLOT(toRem(const QStringList &)));
    connect(this->b, SIGNAL(valueChanged(const QImage &,const QString &)), this, SLOT(addToList(const QImage &,const QString &)), Qt::AutoConnection);
    this->b->start();
    this->DateiListe();
    if(ui->checkBox_changeWPOnStartup->checkState() == Qt::Checked) {
        RandomWallpaper();
    }
    Started = QTime::currentTime();
}

void Widget::setTooltipText(){
    QString Pfad = QFileInfo(this->currentWP()).absolutePath();
    QString Datei = QFileInfo(this->currentWP()).fileName();
    QPixmap currentpix = QPixmap(this->currentWP());
    const QString text = trUtf8("Filename: %1\nPath: %2\nResolution: %3x%4").arg(Datei).arg(Pfad).arg(currentpix.width()).arg(currentpix.height());
    Tray->setToolTip(text);
    currentpix = QPixmap();
    currentpix.detach();
}

void Widget::statisticer(){
    QFile* installed = new QFile(QDir::homePath() + "/.gconf/apps/Cortina/installed");
    
    if(installed->exists() == false) {
        QUrl url = QUrl("http://eric32.er.funpic.de/counter.php?version=" + QApplication::applicationVersion());
        QNetworkAccessManager *manager = new QNetworkAccessManager();
        connect(manager, SIGNAL(finished(QNetworkReply*)), SLOT(counterFinished(QNetworkReply*)));
        QNetworkRequest request = QNetworkRequest(url);
        
        QString var(getenv("http_proxy"));
        QRegExp regex("(http://)?(.*):(\\d*)/?");
        int pos = regex.indexIn(var);
        
        if(pos > -1){
            QString host = regex.cap(2);
            int port = regex.cap(3).toInt();
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, host, port);
            QNetworkProxy::setApplicationProxy(proxy);
        }
        
        manager->get(request);
    }else {
        if ( installed->open(QIODevice::ReadOnly) )
        {
            QByteArray installed_version = installed->readAll();
            if(installed_version != QApplication::applicationVersion()){
                installed->close();
                installed->remove();
                this->statisticer();
            }
        }
        else
        {
            QString DebugString = "Could not open file %1";
            qDebug() << DebugString.arg(installed->fileName());
        }
    }
}

void Widget::counterFinished(QNetworkReply* reply){
    QUrl url = reply->url();
    if (!reply->error()) {
        const QString Cortinagconfdir = QDir::homePath() + "/.gconf/apps/Cortina";
        QDir *gconfCortina = new QDir(Cortinagconfdir);
        if(!gconfCortina->exists()) {
            gconfCortina->mkdir(Cortinagconfdir);
        }
        QFile* installed = new QFile(QDir::homePath() + "/.gconf/apps/Cortina/installed");
        if ( installed->open(QIODevice::WriteOnly) )
        {
            
            installed->write(QApplication::applicationVersion().toUtf8());
            installed->close();
        }
        else
        {
            QString DebugString = "Could not create file %1";
            
            qDebug() << DebugString.arg(installed->fileName());
        }
        
    }
}

void Widget::itemCheckStateChanged(QListWidgetItem* item){
    if(firstload > 0) {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        // ui->listWidget_Dirs->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
        if(stringlist.length() != 0) {
            //for (int i = 0; i<stringlist.length(); i++) {
            //                int bla2 = ui->listWidget_Dirs->selectedItems()[0]->checkState();
            //                QString bla = checkstatelist.at(i);
            //                if(bla.toInt() != bla2) {
            for (int q = 0; q<stringlist.length(); q++) {
                if(item->text() == stringlist.at(q)) {
                    if(item->checkState() == 2) {
                        QString ItemToChange = item->text();
                        for (int q = 0; q<stringlist.length(); q++) {
                            if(stringlist.at(q) != ItemToChange){
                                if(stringlist.at(q).startsWith(ItemToChange) == true) {
                                    
                                    QString dublicateString = trUtf8("%1 would be double monitored, please remove that entry before!")
                                                              .arg(stringlist.at(q));
                                    ui->listWidget_Dirs->selectedItems()[0]->setCheckState(Qt::Unchecked);
                                    QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                                    return;
                                } else {
                                    checkstatelist.replace(q, "2");
                                }
                            } else if(stringlist.length() == 1){
                                checkstatelist.replace(q, "2");
                            }
                        }
                    } else {
                        checkstatelist.replace(q, "0");
                    }
                    Settings->setValue("listWidget_Dirs_flags", checkstatelist);
                    this->DirChanged();
                }
            }
            //                }
            //}
            ui->listWidget_Dirs->setCurrentRow(ui->listWidget_Dirs->currentRow() + 1, QItemSelectionModel::SelectCurrent);
        }
    }
}

void Widget::ThreadShouldStop(){
    this->SaveSettings();
    this->b->terminate();
    delete this->b;
    delete ui;
}

Widget::~Widget()
{
    this->SaveSettings();
    if(b->isRunning()) {
        connect(this->b, SIGNAL(IShouldStop()), this, SLOT(ThreadShouldStop()));
        this->b->shouldIStop = true;
    } else {
        this->SaveSettings();
        this->b->terminate();
        delete this->b;
        delete ui;
    }
}

void Widget::hider(){
    mutex.lock();
    this->b->IShouldSleep = true;
    mutex.unlock();
    this->hide();
    
}

void Widget::setAutoStart(bool set){
    
    QDir autostartdir = QDir(QDir::homePath() + "/.config/autostart");
    if(!autostartdir.exists()){
        if(!autostartdir.mkdir(QDir::homePath() + "/.config/autostart")){
            qWarning() << "Autostart could not be activated: "+ QDir::homePath() + "/.config/autostart" +" could not be created!"  ;
        }
    }
    QFile *startfile = new QFile(QDir::homePath() + "/.config/autostart/cortina.desktop");
    if(set) {
        QByteArray FileData =  QString("[Desktop Entry]\nType=Application\nExec="%QApplication::applicationFilePath()%"\nHidden=false\nNoDisplay=false\nX-GNOME-Autostart-enabled=true\nName=Cortina").toUtf8();
        if(!startfile->exists()) {
            if(startfile->open(QIODevice::WriteOnly)) {
                int returnStartFile = startfile->write(FileData);
                if(returnStartFile != -1 && returnStartFile == FileData.size()) {
                    startfile->flush();
                    startfile->close();
                } else {
                    qDebug() << "Failure while writing to autostart file";
                }
            }
        } else {
            if(startfile->size() != FileData.size()) {
                if(!startfile->remove()) {
                    qDebug() << "cant remove startup file!";
                }
                this->setAutoStart(set);
            }
        }
    } else {
        if(startfile->exists()) {
            if(!startfile->remove()) {
                qDebug() << "cant remove startup file!";
            }
        }
    }
}
void Widget::StartGui(){
    b->start();
    emit(this->shown(waitOnShow));
    this->waitOnShow.clear();
    mutex.lock();
    this->b->IShouldSleep = 0;
    mutex.unlock();
    this->show();
    if(!this->firstTime) {
        this->b->fileschanged = false;
        emit(toAddSignal(Files));
        this->CheckVersion();
        firstTime=true;
    }
}
void Widget::DeleteCurrentWP(){
    QString toRemove = this->currentWP();
    if(!QFileInfo(toRemove).isWritable()) {
        qWarning() << QString("Couldn't remove %1  -  insufficent permissions.").arg(toRemove);
    } else {
        if(toRemove != ""){
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText(trUtf8("Do you really want to delete this image from disk?"));
            msgBox->setStandardButtons(QMessageBox::Yes |QMessageBox::No);
            connect(msgBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(YesDeleteCurrentWP(QAbstractButton*)), Qt::AutoConnection);
            msgBox->show();
        }  else {
            return;
        }
    }
}
void Widget::YesDeleteCurrentWP(QAbstractButton* button){
    QString toRemove = this->currentWP();
    QString Pfad = QFileInfo(toRemove).absolutePath();
    
    QString Datei = QFileInfo(toRemove).fileName();
    if(button->text() ==  "&Yes") {
        
        this->RandomWallpaper();
        
        if(QFile(toRemove).remove()) {
            //   QStringList remlist;
            //    remlist.append(toRemove);
            //   toRem(toRemove);
            Tray->showMessage(trUtf8("Successful removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 5000);
        } else {
            if(QFile(toRemove).exists()){
                qWarning() << QString("Couldn't remove %1").arg(toRemove);
            } else {
                qWarning() << QString("Couldn't remove %1 because it is not existing anymore.").arg(toRemove);
            }
        }
    }
    if(button->text() ==  "&No") {
        Tray->showMessage(trUtf8("Not removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 2500);
    }
}

void Widget::TrayClick(QSystemTrayIcon::ActivationReason Reason){
    
    switch(Reason)
    {
    case QSystemTrayIcon::Trigger:
        on_timeEdit_timeChanged(ui->timeEdit->time());
        this->RandomWallpaper();
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
        
    }
    
}

void Widget::SaveWpStyle(int stylenum){
    QStringList styles;
    styles << "zoom" << "wallpaper" << "centered" << "scaled" << "stretched" << "spanned";
    QByteArray style = styles[stylenum].toUtf8();
    Settings->setValue("comboBox_wpstyle", stylenum);
    QProcess script(this);
    QString path = "gsettings set org.gnome.desktop.background picture-options \'"+ style +"\'";
    script.execute(path);
}

void Widget::SaveSettings(){
    Settings->setValue("checkBox_changeWPOnStartup", ui->checkBox_changeWPOnStartup->checkState());
    Settings->setValue("checkBox_loadOnStartup", ui->checkBox_loadOnStartup->checkState());
    Settings->setValue("checkBox_loadOnStartup", ui->checkBox_loadOnStartup->checkState());
    Settings->setValue("timeEdit", ui->timeEdit->time());
    Settings->setValue("comboBox_wpstyle", ui->comboBox_wpstyle->currentIndex());
    if(Settings->isWritable() == false) {
        qDebug() << "Cant save Settings, because the path is not writable: " << Settings->fileName();
    }
    Settings->sync();
}


void Widget::LoadSettings(){
    this->ui->comboBox_wpstyle->setCurrentIndex(Settings->value("comboBox_wpstyle").toInt());
    switch(Settings->value("checkBox_changeWPOnStartup").toInt())
    {
    case 2:
        ui->checkBox_changeWPOnStartup->setCheckState(Qt::Checked);
        this->setAutoStart(true);
        break;
    default: ui->checkBox_changeWPOnStartup->setCheckState(Qt::Unchecked);
        this->setAutoStart(false);
    }
    
    switch(Settings->value("checkBox_loadOnStartup").toInt())
    {
    case 2: ui->checkBox_loadOnStartup->setCheckState(Qt::Checked);
        break;
    default: ui->checkBox_loadOnStartup->setCheckState(Qt::Unchecked);
    }
    ui->timeEdit->setTime(Settings->value("timeEdit").toTime());
    this->on_timeEdit_timeChanged(Settings->value("timeEdit").toTime());
    QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
    QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
    
    if(!stringlist.isEmpty()) {
        
        for (int i = 0; i<stringlist.length(); i++) {
            QListWidgetItem *tmpwidget =  new QListWidgetItem(stringlist[i], ui->listWidget_Dirs, 0);
            tmpwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            if(!checkstatelist.isEmpty() && !stringlist.isEmpty()) {
                if(checkstatelist.at(i) == "0")
                {
                    tmpwidget->setCheckState(Qt::Unchecked);
                } else {
                    tmpwidget->setCheckState(Qt::Checked);
                }
            } else {
                qDebug() << "index out of range: checkstatelist \nTry to fix the issue...";
                Settings->remove("listWidget_Dirs");
                Settings->sync();
                if(Settings->value("listWidget_Dirs").type() == QVariant::Invalid) {
                    qDebug() << "The issue should be fixed. \nRetry...";
                    this->DirDialog();
                } else {
                    qDebug() << "The issue couldn't' be fixed. \n Please contact the Developer!";
                }
            }
        }
    } else {
        this->DirDialog();
    }
    this->firstload++;
}
void Widget::adjustSizer(){
    ui->btn_about->adjustSize();
    ui->btn_close->adjustSize();
    ui->btn_DirListAdd->adjustSize();
    ui->btn_DirListRem->adjustSize();
    ui->checkBox_changeWPOnStartup->adjustSize();
    ui->checkBox_loadOnStartup->adjustSize();
    ui->comboBox_wpstyle->adjustSize();
    ui->Display->adjustSize();
    ui->label->adjustSize();
    ui->label_changingInterval->adjustSize();
    ui->label_startoptions->adjustSize();
    ui->label_Style->adjustSize();
    ui->label_title1_listwidget_Dirs->adjustSize();
    ui->label_title2_listwidget_Dirs->adjustSize();
    ui->label_wpDir->adjustSize();
    ui->Preference->adjustSize();
    ui->ThumbCounter->adjustSize();
}

void Widget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        this->adjustSizer();
        break;
    default:
        break;
    }
}

void Widget::about()
{
    //QString License = "This program is free software: you can redistribute it and/or modify\nthe Free Software Foundation, either version 3 of the License, or\n(at your option) any later version.\n\nThis package is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.";
    QMessageBox *about = new QMessageBox(this);
    QString name = QApplication::applicationName();
    QString version = QApplication::applicationVersion();
    QString versiontr = trUtf8("version");
    
    QLabel *website_launchpad = new QLabel(about);
    website_launchpad->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    website_launchpad->setOpenExternalLinks(true);
    website_launchpad->setText("<a href=\"https://launchpad.net/cortina\">Launchpad</a>");
    website_launchpad->setGeometry(35, about->height() + 30, 100, website_launchpad->height());
    
    QLabel *website_sourcforge = new QLabel(about);
    website_sourcforge->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    website_sourcforge->setOpenExternalLinks(true);
    website_sourcforge->setText("<a href=\"http://sourceforge.net/projects/cortina\">Sourceforge</a>");
    website_sourcforge->setGeometry(website_launchpad->x(), website_launchpad->y() + 20, 100, website_sourcforge->height());
    
    QLabel *email = new QLabel(about);
    email->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    email->setOpenExternalLinks(true);
    email->setText("<a href=\"mailto:Eric.Baudach@web.de\">Eric.Baudach@web.de</a>");
    email->setGeometry(website_launchpad->x(), website_launchpad->y() + 40, 200, email->height());
    
    about->setText(name +"\n"+ versiontr +": "+ version +"\nCopyright 2010-2012 Eric Baudach\n\n\n\n");
    about->setWindowTitle(trUtf8("about"));
    about->show();
}


QString Widget::currentWP(){
    QString wp2;
    QProcess script(this);
    QStringList arguments;
    arguments << "get" << "org.gnome.desktop.background" << "picture-uri";
    //connect(&script, SIGNAL(error(QProcess::ProcessError)), &script, SLOT(kill()));
    QProcess script2(this);
    script.start("gsettings", arguments, QIODevice::ReadWrite);
    // Continue reading the data until EOF reached
    QByteArray data;
    while(script.waitForReadyRead())
        data.append(script.readAll());
    wp2 = QUrl(data.data()).toString().remove(0,8).trimmed().remove('\'');
    
    return wp2;
}

void Widget::setWP(QString tempwpfile){
    if(WidgetIcon == false) {
        this->setWindowIcon(QIcon(QString::fromUtf8(":/icons/cortina.svg")));
        this->Tray->setIcon(QIcon(QString::fromUtf8(":/icons/cortina.svg")));
        WidgetIcon = true;
    } else {
        WidgetIcon = false;
        this->setWindowIcon(QIcon(QString::fromUtf8(":/icons/cortina2.svg")));
        this->Tray->setIcon(QIcon(QString::fromUtf8(":/icons/cortina2.svg")));
    }
    // gsettings set org.gnome.desktop.background picture-uri 'file:///home/eric/Bilder/Backgrounds/town-beach-coast-1366-768-6618.jpg'
    QProcess script(this);
    QString path = "gsettings set org.gnome.desktop.background picture-uri \'"+ QUrl::fromLocalFile(tempwpfile).toEncoded(QUrl::None) +"\'";
    QStringList arguments;
    arguments << path;
    
    QProcess script2(this);
    
    script.execute(path);
    
    if(QFileInfo(this->currentWP()).isWritable()) {
        removeAction->setEnabled(true);
    } else {
        removeAction->setEnabled(false);
    }
    setTooltipText();
}

void Widget::addToList(const QImage& image,const QString& filename){
    if(this->b->fileschanged == false) {
        QString fileDir = QFileInfo(filename).absolutePath();
        QString filename2 = QFileInfo(filename).fileName();
        QString thumb = fileDir + "/.thumb_" + filename2;
        QPixmap Pixmap;
        mutex.tryLock();
        if(image.isNull()) {
            Pixmap = QPixmap(thumb);
        } else {
            try{
                Pixmap = QPixmap::fromImage(image);
            } catch (...) {
                qDebug() << filename << " couldn't be loaded!";
                return;
            }
        }
        ItemList.append(filename);
        mutex.unlock();
        QPixmap & refPixmap = Pixmap;
        QIcon Icon = QIcon(refPixmap);
        const QIcon & refIcon = Icon;
        QString filenameWithoutPath = QFileInfo(filename).fileName();
        this->ui->listWidget->addItem(new QListWidgetItem(refIcon, filenameWithoutPath, this->ui->listWidget,0));
        Pixmap = QPixmap();
        Icon = QIcon();
        this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
    }
}
void Widget::YesDeleteSelectedWP(QAbstractButton* button){
    if(button->text() ==  "&Yes") {
        QString toRemove = this->ItemList.at(this->ui->listWidget->currentIndex().row());
        if(!QFileInfo(toRemove).isWritable()) {
            qWarning() << QString("Couldn't remove +1  -  insufficent permissions.").arg(toRemove);
        } else {
            QString Pfad = QFileInfo(toRemove).absolutePath();
            QString Datei = QFileInfo(toRemove).fileName();
            if(QFile(toRemove).remove()) {
                //Tray->setToolTip(trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad));
                Tray->showMessage(trUtf8("Successful removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 5000);
            } else {
                if(QFile(toRemove).exists()){
                    qWarning() << QString("Couldn't remove %1").arg(toRemove);
                } else {
                    qWarning() << QString("Couldn't remove %1 because it is not existing anymore.").arg(toRemove);
                }
            }
        }
    }
}

void Widget::DeleteSelectedWP(){
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setText(trUtf8("Do you really want to delete this image from disk?"));
    msgBox->setStandardButtons(QMessageBox::Yes |QMessageBox::No);
    connect(msgBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(YesDeleteSelectedWP(QAbstractButton*)));
    msgBox->show();
}
void Widget::listWidget_customContextMenuRequested(const QPoint & pointer){
    QAction *Aktion = new QAction(trUtf8("delete"), ui->listWidget);
    connect(Aktion, SIGNAL(triggered()), this, SLOT(DeleteSelectedWP()));
    QMenu menu(ui->listWidget);
    menu.addAction( Aktion );
    menu.exec(ui->listWidget->mapToGlobal( pointer ));
}
void Widget::listWidget_doubleClicked(QModelIndex i)
{
    on_timeEdit_timeChanged(ui->timeEdit->time());
    QString Zeile = ItemList.at(i.row());
    if(QFile(Zeile).exists()) {
        this->setWP(Zeile);
        //  qDebug() << "Changed Wallpaper to:" << Zeile;
    } else {
        qDebug() << "File not found! " << Zeile;
    }
}

void Widget::DirDialog(){
    FD = new QFileDialog(NULL, Qt::Dialog);
    FD->setAcceptMode(QFileDialog::AcceptOpen);
    FD->setLabelText(QFileDialog::Accept, trUtf8("add"));
    FD->setLabelText(QFileDialog::FileType, trUtf8("File typ"));
    FD->setLabelText(QFileDialog::FileName, trUtf8("Directory name"));
    FD->setLabelText(QFileDialog::Reject, trUtf8("close"));
    FD->setWindowTitle(trUtf8("Choose directory..."));
    connect(this->FD, SIGNAL(finished(int)), this, SLOT(FDChangeDir(int)));
    FD->setDirectory(QDir::home());
    FD->setFileMode(QFileDialog::DirectoryOnly);
    FD->exec();
    
}

void Widget::FDChangeDir(int i){
    //   int add = 0;
    if(i != 0) {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        foreach(QString selectedFile, this->FD->selectedFiles())
        {
            
            for(int i = 0; i<stringlist.length(); i++) {
                if(selectedFile.startsWith(stringlist[i]) == true  &&  checkstatelist[i] == "2") {
                    //  add++;
                    QString dublicateString = trUtf8("%1 is already monitored through %2!")
                                              .arg(selectedFile).arg(stringlist[i]);
                    QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                    return;
                } else if(stringlist[i] == selectedFile){
                    //  add++;
                    QString dublicateString = trUtf8("%1 is already monitored!").arg(selectedFile);
                    QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                    return;
                }
            }
            // if(add == 0) {
            QListWidgetItem *tmpwidget = new QListWidgetItem(selectedFile, ui->listWidget_Dirs, 0);
            tmpwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            tmpwidget->setCheckState(Qt::Unchecked);
            stringlist.append(selectedFile);
            checkstatelist.append("0");
            Settings->setValue("listWidget_Dirs", stringlist);
            Settings->setValue("listWidget_Dirs_flags", checkstatelist);
            //this->DateiListe();
            this->DirChanged();
            /*if(Files.count() != 0) {
                for(int i = 0; i < this->ItemList.count(); i++)    {
                    if(Files.contains(ItemList[i])) {
                        Files.removeAll(ItemList[i]);
                    }
                }
            }*/
            this->b->IShouldSleep = false;
            this->b->fileschanged = false;
            //this->DateiListe();
            //emit(toAddSignal(Files));
            this->SaveSettings();
            this->FD->deleteLater();
            
            // }
        }
    }
}


void Widget::DirListRem(){
    
    this->b->IShouldSleep = true;
    DateiListe();
    if(this->ui->listWidget_Dirs->count() != 0) {
        removedItem =  ui->listWidget_Dirs->selectedItems()[0]->text();
        Qt::CheckState checkstate = ui->listWidget_Dirs->selectedItems()[0]->checkState();
        QStringList remove_dirs;
        
        if(checkstate == Qt::Checked){
            for(int i = 0; i < ItemList.length(); i++) {
                if(ItemList.at(i).startsWith(removedItem)){
                    delete this->ui->listWidget->item(i);
                    this->ItemList.removeAt(i);
                    this->ImageSavedWatcher = true;
                }
                this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
            }
            QDir dir(removedItem);
            this->watcher->removePath(removedItem);
            remove_dirs.append(removedItem);
            QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
            foreach(QFileInfo datei, list) {
                this->watcher->removePath(datei.absoluteFilePath());
                remove_dirs.append(datei.absoluteFilePath());
            }
            remove_dirs.clear();
            remove_dirs.append(removedItem);
        }else{
            this->watcher->removePath(removedItem);
            remove_dirs.append(removedItem);
            for(int i = 0; i < ItemList.length(); i++) {
                
                if(ItemList.at(i).startsWith(removedItem)){
                    delete this->ui->listWidget->item(i);
                    this->ItemList.removeAt(i);
                    this->ImageSavedWatcher = true;
                    this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
                }
            }
        }
        emit(helper_remove_dir_thumbs_signal(checkstate,remove_dirs));
        delete ui->listWidget_Dirs->selectedItems()[0];
        this->b->fileschanged = true;
    }
    if(this->ui->listWidget_Dirs->count() == 0) {
        this->ItemList.clear();
        this->ui->listWidget->clear();
        this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
        this->Files.clear();
    }
    
    Settings->setValue("listWidget_Dirs", stringlist);
    Settings->setValue("listWidget_Dirs_flags", checkstatelist);
    /* QStringList tmpremFiles;
        QDir verzeichnis = QDir(removedItem);
        const QStringList & SubDirs = verzeichnis.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        foreach(QString File, Files) {
            if(SubDirs.count() != 0){
                int helperCount = 0;
                for(int i = 0; i<SubDirs.count(); i++) {
                    if(!File.contains(verzeichnis.path() + "/" + SubDirs[i]))
                    {
                        helperCount++;
                    }
                }
                int bla = SubDirs.count() - helperCount;
                if(bla == 0 && File.contains(removedItem)) {
                    tmpremFiles.append(File);
                }
                
            }else{
                if(File.contains(removedItem)) {
                    tmpremFiles.append(File);
                }
            }
        }
        if(tmpremFiles.count() != 0){
            DateiListe();
            const QStringList & remFiles = tmpremFiles;
            toRem(remFiles);
        }
    } else {
   Settings->remove("listWidget_Dirs");
   Settings->remove("listWidget_Dirs_flags");
   this->ItemList.clear();
   this->ui->listWidget->clear();
   this->Counter = 0;
   this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
   this->Files.clear();
                }*/
    this->b->IShouldSleep = false;
}


void Widget::CheckVersion(){
    QUrl url = QUrl("http://eric32.er.funpic.de/cortina/natty_current");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*))
            , SLOT(downloadFinished(QNetworkReply*)));
    
    QString var(getenv("http_proxy"));
    QRegExp regex("(http://)?(.*):(\\d*)/?");
    int pos = regex.indexIn(var);
    
    if(pos > -1){
        QString host = regex.cap(2);
        int port = regex.cap(3).toInt();
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, host, port);
        QNetworkProxy::setApplicationProxy(proxy);
    }
    
    //qDebug() << QNetworkProxy::applicationProxy().hostName();
    QNetworkRequest request = QNetworkRequest(url);
    manager->get(request);
}

void Widget::downloadFinished(QNetworkReply* reply){
    QUrl url = reply->url();
    if (reply->error()) {
        qDebug() << "Version check failed: " + reply->errorString();
    } else if(reply->isReadable() && reply->size() != 0) {
        QByteArray buf = reply->readAll();
        QString inhalt;
        for(int i = 0; i<buf.length(); i++) {
            inhalt += buf.at(i);
        }
        inhalt = inhalt.trimmed();
        QString InstalledVersion = QApplication::applicationVersion();
        if(inhalt != InstalledVersion) {
            QStringList splittedlocal = InstalledVersion.split(".");
            QStringList splittedRemote = inhalt.split(".");
            int local = 0;
            int remote = 0;
            int i = 0;
            foreach(QString num, splittedlocal) {
                if(i == 0)
                {
                    local = local + num.toInt() * 100;
                }
                if(i == 1)
                {
                    local = local + num.toInt() * 10;
                } else{
                    local = local + num.toInt();
                }
                i++;
            }
            i = 0;
            foreach(QString num, splittedRemote) {
                if(i == 0)
                {
                    remote = remote + num.toInt() * 100;
                }
                if(i == 1)
                {
                    remote = remote + num.toInt() * 10;
                } else{
                    remote = remote + num.toInt();
                }
                i++;
            }
            qDebug() << "remote=" << remote << "\n" << "local=" << local;
            if(remote > local) {
                
                
                QMessageBox *version = new QMessageBox(QMessageBox::Information, trUtf8("Update available!"), trUtf8("%1 is available!").arg(inhalt) + "\n\n\n\n\n\n\n", QMessageBox::Ok, this, Qt::Tool);
                
                QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
                QGridLayout* layout = (QGridLayout*)version->layout();
                layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
                
                QLabel *dl_launchpad = new QLabel(version);
                dl_launchpad->setScaledContents(true);
                dl_launchpad->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
                dl_launchpad->setOpenExternalLinks(true);
                dl_launchpad->setText("<a href=\"https://launchpad.net/cortina/+download\">"+trUtf8("Download from Launchpad")+"</a>");
                dl_launchpad->setGeometry(10, version->height() + 15, 400, 60);
                
                QLabel *ppa = new QLabel(version);                
                ppa->setScaledContents(true);
                ppa->setTextInteractionFlags(Qt::TextSelectableByMouse);
                ppa->setOpenExternalLinks(true);
                ppa->setText(trUtf8("Add my PPA") + ":\nsudo add-apt-repository ppa:cs-sniffer/cortina");
                ppa->setGeometry(dl_launchpad->x(), dl_launchpad->y() + 40, 400, 60);
                version->updateGeometry();
                version->show();
                version->updateGeometry();
            }
        }
    }
    return;
    reply->deleteLater();
}

void Widget::CreateUI(){
    this->adjustSizer();
    connect(ui->btn_DirListAdd, SIGNAL(clicked()), this, SLOT(DirDialog()));
    connect(ui->btn_DirListRem, SIGNAL(clicked()), this, SLOT(DirListRem()));
    intervalTimer = new QTimer(this);
    connect(intervalTimer, SIGNAL(timeout()), this, SLOT(RandomWallpaper()));
    ui->Tab->setCurrentIndex(0);
}

QStringList Widget::DateiListeNeu(){
    QStringList FilesNeu;
    QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
    QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
    if(stringlist.length() != 0) {
        if(Settings->value("listWidget_Dirs").toStringList().length() != 0) {
            for (int i = 0; i<stringlist.length(); i++) {
                
                QString picdir =  stringlist.at(i);
                QDir tempDir = QDir(picdir);
                QString picdir_bash_conform = tempDir.canonicalPath().replace(" ", "\ ");
                
                if(checkstatelist.at(i)  == "2") {
                    this->Find_Command = "find \""+picdir_bash_conform+"\" -type f \( -name \"*.jpeg\" -o -name \"*.gif\" -o -name \"*.jpg\" -o -name \"*.png\" -o -name \"*.svg\" \) ! -name \".thumb_*\"";
                    QString findDirs = "find \""+picdir_bash_conform+"\" -type d";
                    QProcess process;
                    process.setProcessChannelMode(QProcess::MergedChannels);
                    process.start(findDirs ,QIODevice::ReadOnly);
                    
                    // Continue reading the data until EOF reached
                    QByteArray DirData;
                    
                    while(process.waitForReadyRead())
                        DirData.append(process.readAll());
                    
                    // Output the data
                    qDebug( "%s", DirData.data());
                    qDebug("Done!");
                    if(DirData.length() != 0){
                        QList<QByteArray> dataList = DirData.split('\n');
                        
                        // QList<QByteArray> to QStringList
                        for(int i=0; i<dataList.size(); ++i){
                            qDebug() << dataList[i];
                            if(dataList[i].length() != 0){
                                if(!watcher->directories().contains(dataList[i])) {
                                    watcher->addPath(dataList[i]);
                                }
                            }
                        }
                    }
                } else {
                    this->Find_Command = "find \""+picdir_bash_conform+"\" -maxdepth 1 -type f \( -name \"*.jpeg\" -o -name \"*.gif\" -o -name \"*.jpg\" -o -name \"*.png\" -o -name \"*.svg\" \) ! -name \".thumb_*\"";
                    if(!watcher->directories().contains(picdir)) {
                        watcher->addPath(picdir);
                    }
                }
                QProcess process;
                process.setProcessChannelMode(QProcess::MergedChannels);
                process.start(this->Find_Command ,QIODevice::ReadOnly);
                
                // Continue reading the data until EOF reached
                QByteArray data;
                
                while(process.waitForReadyRead())
                    data.append(process.readAll());
                
                // Output the data
                qDebug( "%s", data.data() );
                qDebug("Done!");
                if(data.length() != 0){
                    QList<QByteArray> dataList = data.split('\n');
                    
                    // QList<QByteArray> to QStringList
                    for(int i=0; i<dataList.size(); ++i){
                        qDebug() << dataList[i];
                        if(dataList[i].length() != 0){
                            FilesNeu.append(dataList[i]);
                        }
                        
                    }
                }
                
            }
        }
    }
    return FilesNeu;
    
}

void Widget::DateiListe(){
    Files.clear();
    if(!Settings->value("listWidget_Dirs").toStringList().isEmpty()) {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        if(stringlist.length() != 0) {
            if(Settings->value("listWidget_Dirs").toStringList().length() != 0) {
                for (int i = 0; i<stringlist.length(); i++) {
                    QString picdir =  stringlist.at(i);                    
                    QDir tempDir = QDir(picdir);
                    QString picdir_bash_conform = tempDir.canonicalPath().replace(" ", "\ ");

                    if(checkstatelist.at(i)  == "2") {
                        this->Find_Command = "find \""+picdir_bash_conform+"\" -type f \( -name \"*.jpeg\" -o -name \"*.gif\" -o -name \"*.jpg\" -o -name \"*.png\" -o -name \"*.svg\" \) ! -name \".thumb_*\"";
                        QString findDirs = "find \""+picdir_bash_conform+"\" -type d";
                        QProcess process;
                        process.setProcessChannelMode(QProcess::MergedChannels);
                        process.start(findDirs ,QIODevice::ReadOnly);
                        
                        // Continue reading the data until EOF reached
                        QByteArray DirData;
                        
                        while(process.waitForReadyRead())
                            DirData.append(process.readAll());
                        
                        // Output the data
                        qDebug( "%s", DirData.data() );
                        qDebug("Done! DateiListe");
                        if(DirData.length() != 0){
                            QList<QByteArray> dataList = DirData.split('\n');
                            
                            // QList<QByteArray> to QStringList
                            for(int i=0; i<dataList.size(); ++i){
                                qDebug() << dataList[i];
                                if(dataList[i].length() != 0){
                                    if(!watcher->directories().contains(dataList[i])) {
                                        watcher->addPath(dataList[i]);
                                    }
                                }
                            }
                        }
                    } else {
                        this->Find_Command = "find \""+picdir_bash_conform+"\" -maxdepth 1 -type f \( -name \"*.jpeg\" -o -name \"*.gif\" -o -name \"*.jpg\" -o -name \"*.png\" -o -name \"*.svg\" \) ! -name \".thumb_*\"";
                        if(!watcher->directories().contains(picdir)) {
                            watcher->addPath(picdir);
                        }
                    }
                    QProcess process;
                    process.setProcessChannelMode(QProcess::MergedChannels);
                    process.start(this->Find_Command ,QIODevice::ReadOnly);
                    
                    // Continue reading the data until EOF reached
                    QByteArray data;
                    
                    while(process.waitForReadyRead())
                        data.append(process.readAll());
                    
                    // Output the data
                    qDebug( "%s", data.data() );
                    qDebug("Done! DateiListe");
                    if(data.length() != 0){
                        QList<QByteArray> dataList = data.split('\n');
                        
                        // QList<QByteArray> to QStringList
                        for(int i=0; i<dataList.size(); ++i){
                            qDebug() << dataList[i];
                            if(dataList[i].length() != 0){
                                Files.append(dataList[i]);
                            }
                            
                        }
                    }
                    
                }
            }
        }
    }
    if(Files.length() == 0) {
        qDebug() << "Can't work without wallpapers!";
        return;
    }
}
void Widget::ImageSaved(bool saved){
    this->ImageSavedWatcher = saved;
}

//void Widget::LoopWatcher(){
//    qDebug() << Started;
//    qDebug() << QTime::currentTime().addSecs(-5);
//    while( Started < QTime::currentTime().addSecs(-5) ) {
//        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//    }
//    //watcher->startTimer(5);
//            Started = QTime::currentTime();
//}

void Widget::DirChanged(){
    // this->LoopWatcher();
    
    if(this->ImageSavedWatcher == false) {
        qDebug() << ImageSavedWatcher;
        // this->b->fileschanged = true;
        QTime dieTime = QTime::currentTime().addMSecs(100);
        qDebug() << "Loop start..";
        qDebug() << QTime::currentTime().msec();
        qDebug() <<  dieTime.msec();
        
        while( QTime::currentTime() < dieTime ) {
        }
        qDebug() << "Loop end..";
        qDebug() << QTime::currentTime().msec();
        qDebug() <<  dieTime.msec();
        const QStringList & NeueListe = DateiListeNeu();
        const QStringList & AlteListe = Files;
        const int & NeueAnzahl = NeueListe.count();
        const int & AlteAnzahl = AlteListe.count();
        QStringList addy;
        const QStringList & refaddy = addy;
        QStringList remy;
        const QStringList & refremy = remy;
        addy.clear();
        remy.clear();
        
        qDebug() << "NeueAnzahl: " << NeueAnzahl;
        qDebug() << "AlteAnzahl: " << AlteAnzahl;
        qDebug() << "----";
        int NeueAnzahltmp = NeueAnzahl;
        if(NeueAnzahl > AlteAnzahl) {
            // Datei wurde hinzugefügt:
            foreach(QString Datei, NeueListe)
            {
                if(NeueAnzahltmp != AlteAnzahl) {
                    if(AlteListe.indexOf(Datei) == -1) {
                        addy.append(Datei);
                        NeueAnzahltmp--;
                    }
                }
            }
            for(int i = 0; i < this->ItemList.count(); i++)    {
                if(refaddy.contains(ItemList[i])) {
                    addy.removeAll(ItemList[i]);
                }
            }
            if(!this->isHidden()) {
                this->b->fileschanged = false;
                emit(toAddSignal(refaddy));
                this->Files.append(refaddy);
            } else {
                waitOnShow.append(refaddy);
                this->Files.append(refaddy);
            }
        } else if(NeueAnzahl < AlteAnzahl) {
            // Datei wurde gelöscht:
            foreach(QString Datei, AlteListe)
            {
                if(NeueListe.indexOf(Datei) == -1) {
                    remy.append(Datei);
                }
            }
            this->toRem(refremy);
        } else if(NeueAnzahl == AlteAnzahl) {
            foreach(QString Datei, AlteListe)
            {
                if(!QFile(Datei).exists()) {
                    remy.append(Datei);
                }/*else{
                    qDebug()<< this->ItemList.contains(QFileInfo(Datei).absoluteFilePath());
                     qDebug()<< refaddy.contains(Datei);
                    if(NeueListe.contains(Datei) && !refaddy.contains(Datei) && !this->ItemList.contains(QFileInfo(Datei).absoluteFilePath())){
                        addy.append(Datei);
                        if(!this->isHidden()) {
                            this->b->fileschanged = false;
                            emit(toAddSignal(refaddy));
                            this->Files.append(refaddy);
                        } else {
                            waitOnShow.append(refaddy);
                            this->Files.append(refaddy);
                        }
                    }
                }*/
            }
            this->toRem(refremy);
        }
    }
    //qDebug() << "DirChanged!";
    // qDebug() << watcher->directories();
}
void Widget::RemoveFromUI(QByteArray data){
    QList<QByteArray> Liste = data.split('\n');
    Liste.removeLast();
    for(int i=0; i<Liste.size(); ++i){
        QString PicFile = Liste[i].remove(Liste[i].indexOf(".thumb_"), 7);
        QRegExp regex = QRegExp("^"+QFileInfo(Liste[i]).absolutePath()+"$");
        QString  PicFileWidget = QFileInfo(Liste[i]).fileName();
        for(int z=0; z<this->ui->listWidget->findItems(QFileInfo(Liste[i]).fileName(), Qt::MatchEndsWith).length(); ++z){
            qDebug() << this->ui->listWidget->findItems(QFileInfo(Liste[i]).fileName(), Qt::MatchEndsWith)[z]->data(Qt::DisplayRole);
            delete this->ui->listWidget->findItems(QFileInfo(Liste[i]).fileName(), Qt::MatchEndsWith)[z];
            int iterator = ItemList.indexOf(QRegExp("^"+QFileInfo(Liste[i]).absolutePath()+"$"));
            this->ItemList.removeAt(iterator);
            this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
        }
        this->ui->listWidget->update();
    }
}

void Widget::toRem(const QStringList & Liste){
    // emit(ThreadShouldSleep(5));
    QStringList remove_files;
    foreach(QString remDatei, Liste) {
        for(int i = 0; i < ItemList.length(); i++) {
            if(this->ItemList.at(i) == remDatei) {
                delete this->ui->listWidget->item(i);
                this->ItemList.removeAt(i);
                this->ImageSavedWatcher = true;
                remove_files.append(remDatei);
                this->ui->ThumbCounter->setNum(this->ui->listWidget->count());
            }
        }
    }
    emit(helper_remove_file_thumbs_signal(remove_files));
    this->DateiListe();
    this->b->IShouldSleep = false;
    this->ImageSavedWatcher = false;
}




void Widget::RandomWallpaper(){
    QString WP;
    const int & reflenght =   this->Files.length();
    if(reflenght != 0) {
        //if(Settings->value("randomOrNot") == 2) {
        srand((unsigned)time(0));
        int random_integer = (rand()%reflenght);
        const int & refrandom_integer = random_integer;
        WP = Files[refrandom_integer];
        /* } else {
            if((current) != reflenght) {
                WP = Files[current];
                current++;
            } else {
                current = 0;
                WP = Files[current];
            }
       }*/
    } else {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        if(!stringlist.isEmpty()) {
            //this->DateiListe();
            if(Files.length() == 0) {
                return;
            }
            this->RandomWallpaper();
        }
    }
    if(QFile(WP).exists()) {
        this->setWP(WP);
    } else {
        qDebug() << "File not found! " << WP;
    }
}

void Widget::on_timeEdit_timeChanged(QTime date)
{
    float hours2ms = 0;
    float min2ms = 0;
    float sec2ms = 0;
    if(date.hour() > 0){
        hours2ms = ((date.hour() * 60) * 60) * 1000;
    }
    if(date.minute() > 0){
        min2ms = (date.minute() * 60) * 1000;
    }
    if(date.second() > 0){
        sec2ms = date.second() * 1000;
    }
    int msecs = hours2ms + min2ms + sec2ms;
    //qDebug() << msecs;
    
    if(intervalTimer->isActive())
    {
        if(msecs != 0){
            intervalTimer->setInterval(msecs);
        } else {
            intervalTimer->stop();
        }
    } else {
        if(msecs != 0){
            intervalTimer->start(msecs);
        }
    }
}

