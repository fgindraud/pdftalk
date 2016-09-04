#!/usr/bin/env bash
set -xue

# Target is 32b
if [ "$BITS" = 32 ]; then
	MXE_TARGET=i686-w64-mingw32.static
elif [ "$BITS" = 64 ]; then
	MXE_TARGET=x86-64-w64-mingw32.static
else
	exit 1;
fi
MXE_EXEC="${MXE_TARGET/x86-64/x86_64}"
MXE_DIR=/usr/lib/mxe

### Get QT and binutils ###

# Add MXE ppa
echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
	| sudo tee /etc/apt/sources.list.d/mxeapt.list
sudo apt-key adv --keyserver keyserver.ubuntu.com \
	--recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB

# Install cross compiled Qt and binutils (and upx for binary compression)
# Also install poppler deps, but not poppler itself as it isn't compiled with qt5 support.
sudo apt-get -y -q update
sudo apt-get -y -q install \
	upx-ucl \
	mxe-${MXE_TARGET}-qtbase \
	mxe-${MXE_TARGET}-binutils \
	mxe-${MXE_TARGET}-{gcc,cairo,curl,freetype,glib,jpeg,lcms,libpng,tiff,zlib}

# Redirect to right qmake and g++
export PATH="${MXE_DIR}/usr/${MXE_EXEC}/qt5/bin:${PATH}"
export PATH="${MXE_DIR}/usr/bin:${PATH}"

# Manually cross compiled and install poppler
POPPLER_VER=0.30.0 # old, matching stuff on MXE
POPPLER_DIR=poppler-${POPPLER_VER}
POPPLER_TAR=${POPPLER_DIR}.tar.xz

wget -q --no-check-certificate "http://poppler.freedesktop.org/${POPPLER_TAR}"
tar xf ${POPPLER_TAR}
cd ${POPPLER_DIR}

./configure \
	--host=${MXE_EXEC} \
	--prefix=${MXE_DIR}/usr/${MXE_EXEC} \
	\
	--enable-static --disable-shared \
	--enable-poppler-qt5 --disable-poppler-qt4 \
	\
	--disable-silent-rules \
	--enable-xpdf-headers \
	--enable-zlib \
	--enable-cms=lcms2 \
	--enable-libcurl \
	--enable-libtiff \
	--enable-libjpeg \
	--enable-libpng \
	--enable-poppler-glib \
	--enable-poppler-cpp \
	--enable-cairo-output \
	--enable-splash-output \
	--enable-compile-warnings=yes \
	--enable-introspection=auto \
	--disable-libopenjpeg \
	--disable-gtk-test \
	--disable-utils \
	--disable-gtk-doc \
	--disable-gtk-doc-html \
	--disable-gtk-doc-pdf \
	--with-font-configuration=win32 \
	CXXFLAGS=-D_WIN32_WINNT=0x0500 \
	LIBTIFF_LIBS="`${MXE_EXEC}-pkg-config libtiff-4 --libs`"
make
make install

set +xue
