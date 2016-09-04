#!/usr/bin/env bash
set -xue

# Get deps (Qt, Poppler, upx for binary packing)
sudo apt-get -y -q update
sudo apt-get -y -q install \
	upx-ucl \
	qt5-default \
	qt5-qmake \
	libpoppler-qt5-dev

set +xue
