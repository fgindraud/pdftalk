### Compilation ###

TEMPLATE = app
CONFIG += c++11

INCLUDEPATH += src/
CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

QT += core widgets
HEADERS +=
SOURCES += \
	src/main.cpp

### Poppler ###

unix:!macx { # Linux
	CONFIG += link_pkgconfig
	PKGCONFIG += poppler-qt5
}
macx: {
	# TODO
}
win32: {
	# TODO
}

### Misc information ###

VERSION = 0.0.1
DEFINES += PDFTALK_VERSION=$${VERSION}

QMAKE_TARGET_COMPANY = Francois Gindraud
QMAKE_TARGET_PRODUCT = PDFTalk
QMAKE_TARGET_DESCRIPTION = PDF presentation tool
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2016 Francois Gindraud

