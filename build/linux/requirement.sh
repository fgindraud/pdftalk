#!/usr/bin/env bash
set -xue

# Get deps (Qt, Poppler)
sudo apt-get -y -q update
sudo apt-get -y -q install \
	qt5-default \
	qt5-qmake \
	libpoppler-qt5-dev

set +xue
