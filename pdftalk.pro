### Compilation ###

TEMPLATE = app
CONFIG += c++11

INCLUDEPATH += src/
CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

QT += core widgets
HEADERS += \
	src/document.h \
	src/cache.h \
	src/pixmap_label.h \
	src/presentation.h \
	src/presentation_window.h \
	src/presenter_window.h \
	src/window_pair.h \
	src/main.h
SOURCES += \
	src/main.cpp

# Poppler
macx: { # Mac
	# Stack overflow : pkg config disabled by default on mac...
	QT_CONFIG -= no-pkg-config
}
CONFIG += link_pkgconfig
PKGCONFIG += poppler-qt5

### Misc information ###

VERSION = 0.0.2
DEFINES += PDFTALK_VERSION=$${VERSION}

QMAKE_TARGET_COMPANY = Francois Gindraud
QMAKE_TARGET_PRODUCT = PDFTalk
QMAKE_TARGET_DESCRIPTION = PDF presentation tool
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2016 Francois Gindraud

