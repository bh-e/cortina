/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2012 Eric Baudach>
 */

#include "threadb.h"
#include "widget.h"
#include "stdio.h"

threadb::threadb(QWidget *parent) :
    QThread(parent)
{
    QObject::moveToThread(this);
    shouldIStop = false;
    IShouldSleep = false;
    fileschanged = false;
}

void threadb::sleeping(int sec){
    this->sleep(sec);
}

void threadb::FileAdded(const QStringList & Files){
    QImage image = QImage();
    const QImage & refimage = image;
    if(!shouldIStop) {

        const int & reflenght = Files.length();
        for(int i = 0; i < reflenght; i++) {
            const QString & reffile = Files.at(i);

            if(fileschanged == true) {
                i = reflenght -1;
            }
            QString fileDir = QFileInfo(reffile).absolutePath();
            QString filename = QFileInfo(reffile).fileName();
            QString thumb = fileDir % "/.thumb_" % filename;

            if(!QFile(thumb).exists()) {
                while(IShouldSleep == true) {
                    this->sleep(2);
                }
                mutex.lock();
                try {
                    image = QImage(reffile).scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                } catch (...) {
                    qDebug() << reffile << " couldn't be loaded!";
                    i++;
                }

                const QStringList & refFiles = Files;
                if(fileschanged == true) {
                    i = reflenght -1;
                }
                if(!image.isNull()) {
                    mutex.unlock();
                    Q_EMIT valueChanged(refimage,refFiles[i]);
                    Q_EMIT(ImageSaved(true));
                    if(!refimage.save(thumb, 0, 100)) {
                        qDebug() << thumb << " couldn't be saved!";
                    }
                    if(!image.isNull()) {
                        mutex.lock();
                        image = QImage();
                        mutex.unlock();
                    }
                } else {
                    mutex.unlock();
                    qDebug() << "Failed to load " << refFiles[i];
                }
            } else {
                image = QImage();
                Q_EMIT valueChanged(refimage,reffile);
            }
        }
        Q_EMIT(ImageSaved(false));
    } else {
        Q_EMIT(IShouldStop());
    }

}

void threadb::run()
{
    exec();
}

