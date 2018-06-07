#!/usr/bin/env bash
set -xue

# Install qt5 and poppler from brew
brew install qt5
brew unlink python # Python 2 is installed as dep of poppler, and fails to override symlinks, failing the whole build.
brew install poppler --with-qt

# Add path to find qmake (that will handle all other paths)
export PATH="$(brew --prefix qt5)/bin:${PATH}"

set +xue
