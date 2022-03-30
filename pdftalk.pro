### Compilation ###

TEMPLATE = app
CONFIG += c++11

INCLUDEPATH += src/
CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

QT += core widgets
HEADERS += \
	src/action.h \
	src/controller.h \
	src/document.h \
	src/render.h \
	src/render_internal.h \
	src/utils.h \
	src/views.h \
	src/window.h
SOURCES += \
	src/action.cpp \
	src/controller.cpp \
	src/document.cpp \
	src/main.cpp \
	src/prefetch_strategies.cpp \
	src/render.cpp \
	src/views.cpp

# Poppler
macx: { # Mac
	# Stack overflow : pkg config disabled by default on mac...
	QT_CONFIG -= no-pkg-config
}
CONFIG += link_pkgconfig
PKGCONFIG += poppler-qt5

### Misc information ###

VERSION = 1.0
DEFINES += PDFTALK_VERSION=$${VERSION}

QMAKE_TARGET_COMPANY = Francois Gindraud
QMAKE_TARGET_PRODUCT = PDFTalk
QMAKE_TARGET_DESCRIPTION = PDF presentation tool
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2016 - 2022 Francois Gindraud

