/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2012 Eric Baudach>
 */

#ifndef THREADB_H
#define THREADB_H

#include <QString>
#include <QDir>
#include <QImage>
#include <QStringList>
#include <QIcon>
#include <QWidget>
#include <QThread>
#include <QListWidgetItem>
#include <QList>
#include <QStringList>
#include <QPair>
#include <QMetaType>
#include <QMutex>

class threadb : public QThread
{
    Q_OBJECT
public:
    explicit threadb(QWidget *parent = 0);
    virtual void run();
    bool shouldIStop;
    bool IShouldSleep;
    bool fileschanged;

private:
    QMutex mutex;

Q_SIGNALS:
    void valueChanged(const QImage&,const QString&);
    void dublicateadd(QString);
    void IShouldStop();
    void ImageSaved(bool);

public Q_SLOTS:
    void FileAdded(const QStringList &);
    void sleeping(int);


};

#endif // THREADB_H
