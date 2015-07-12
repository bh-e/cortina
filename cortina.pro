# -------------------------------------------------
# Project created by QtCreator 2010-05-20T11:56:49
# -------------------------------------------------
TARGET = cortina
TEMPLATE = app
SOURCES += main.cpp \
    threadb.cpp \
    widget.cpp \
    helper.cpp
HEADERS += widget.h \
    threadb.h \
    helper.h
FORMS += widget.ui
CONFIG += link_pkgconfig \
    qt \
    release
#PKGCONFIG += libnotify
QT += network
TRANSLATIONS += po/ru.ts \
po/tr.ts \
po/si.ts \
po/nl.ts \
po/fa.ts \
po/uk.ts \
po/cs.ts \
po/kk.ts \
po/de.ts \
po/ca.ts \
po/fr.ts \
po/ko.ts \
po/ar.ts \
po/th.ts \
po/fi.ts \
po/pt_BR.ts \
po/es.ts \
po/ia.ts \
po/pt.ts \
po/pl.ts \
po/it.ts \
po/sv.ts \
po/zh_CN.ts 
icon.path = /usr/share/pixmaps
icon.files += cortina.svg \
cortina2.svg
target.path = /usr/bin
INSTALLS += target
desktop.path = /usr/share/applications
desktop.files = cortina.desktop
INSTALLS += desktop
languages.path = /usr/share/qt4/translations/cortina
languages.extra = $(QTDIR)lrelease -silent Cortina.pro
INSTALLS += languages
translations.path = /usr/share/qt4/translations/cortina
translations.files = po/*.qm
INSTALLS += translations
RESOURCES += \
    rc.qrc
CODECFORSRC = UTF-8
CONFIG(debug) {
    message(debug)
}
CONFIG(release) {
    message(release)
}
CONFIG(debug, debug|release) {
    message("debug, debug|release")
}
CONFIG(release, debug|release) {
    message("release, debug|release")
}
