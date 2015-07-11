/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2012 Eric Baudach>
 */

#include "helper.h"
#include <QProcess>
#include "widget.h"

helper::helper(QWidget *parent) :
        QThread(parent)
{
    QObject::moveToThread(this);
}


void helper::run()
{
    exec();
}

void helper::Remove_File_thumb(const QStringList & Files){
    if(Files.length() != 0){
        Q_EMIT(ImageSaved(true));
        Q_FOREACH(QString file, Files) {
            QString thumb = QFileInfo(file).absolutePath() + "/.thumb_" + QFileInfo(file).fileName();
            QFile::remove(thumb);
        }
        Q_EMIT(ImageSaved(false));
    }
}

void helper::Remove_Dir_thumb(int recursive, const QStringList & Paths) {
    Q_EMIT(ImageSaved(true));
    if(recursive == 2){
        if(Paths.length() != 0){
            Q_FOREACH(QString path, Paths) {
                QDir tempDir = QDir(path);
                QString path_bash_conform = tempDir.canonicalPath().replace(" ", "\ ");
                QString find = "find \""+path_bash_conform+"\" -type f -name .thumb_* -print -exec rm {} \;";

                qDebug() << find;
                QProcess process;
                process.setProcessChannelMode(QProcess::MergedChannels);
                process.start(find ,QIODevice::ReadOnly);

                // Continue reading the data until EOF reached
                QByteArray data;

                while(process.waitForReadyRead())
                    data.append(process.readAll());

                // Output the data
                qDebug(data.data());
                qDebug("Done!");
                if(data.length() != 0){

                    Q_EMIT(RemoveFromUI_signal(data));

                }
            }
        }
    }else{
        if(Paths.length() != 0){
            Q_FOREACH(QString path, Paths) {
                QDir tempDir = QDir(path);
                QString path_bash_conform = tempDir.canonicalPath().replace(" ", "\ ");
                QString find = "find \""+path_bash_conform+"\" -maxdepth 1 -type f -name .thumb_* -print -exec rm {} \;";

                qDebug() << find;
                QProcess process;
                process.setProcessChannelMode(QProcess::MergedChannels);
                process.start(find ,QIODevice::ReadOnly);

                // Continue reading the data until EOF reached
                QByteArray data;

                while(process.waitForReadyRead())
                    data.append(process.readAll());

                // Output the data
                qDebug(data.data());

                qDebug("Done!");
                if(data.length() != 0){
                    Q_EMIT(RemoveFromUI_signal(data));

                }
            }
        }       
    }
    Q_EMIT(ImageSaved(false));
}
