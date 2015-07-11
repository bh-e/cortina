/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2012 Eric Baudach>
 */

#include <QtGui/QApplication>
#include <widget.h>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setEffectEnabled(Qt::UI_AnimateCombo, true);
    a.setEffectEnabled(Qt::UI_AnimateMenu, true);
    a.setEffectEnabled(Qt::UI_FadeTooltip, true);
    a.setEffectEnabled(Qt::UI_FadeMenu, true);

#ifdef QT_DEBUG
    const QString & Translation_path = a.applicationDirPath() + "/po";
    qDebug() << "Load translations from: " + Translation_path;
#else
    const QString & Translation_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/cortina";
#endif

    QTranslator qtTranslator;
    if(!qtTranslator.load(QLocale::system().name(), Translation_path)) {
        if(QLocale::system().name().startsWith("en_") == false) {
            qDebug() << "Can't load translation file:";
            qDebug() << QLocale::system().name() + ".qm in " + QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/cortina";
        }
    }
    a.installTranslator(&qtTranslator);
    a.setApplicationName("Cortina");
    a.setApplicationVersion("1.1.1");
    Widget w;
    return a.exec();
}
