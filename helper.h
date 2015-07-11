#ifndef HELPER_H
#define HELPER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QWidget>
#include <QStringList>
#include <QString>
#include <QDir>
#include <QFile>
#include <QMetaType>


class helper : public QThread
{
    Q_OBJECT
public:
    explicit helper(QWidget *parent = 0);
    virtual void run();
    QString recursiv;
    QStringList paths;
    QStringList files;

public Q_SLOTS:
    void Remove_File_thumb(const QStringList &);
    void Remove_Dir_thumb(int recursive, const QStringList &Paths);

Q_SIGNALS:
    void ImageSaved(bool);
    void RemoveFromUI_signal(QByteArray);

};


#endif // HELPER_H
