#!/usr/bin/env bash
set -xue

# Install qt5 and poppler from brew
# brew update (disabled for test)
brew install qt5
brew install poppler --with-qt5

# Add path to find qmake (that will handle all other paths)
export PATH="$(brew --prefix qt5)/bin:${PATH}"

set +xue
