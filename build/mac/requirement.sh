#!/usr/bin/env bash
set -xue

# Install qt5 and poppler from brew
brew install qt5
brew install 'python@2' || brew link --overwrite 'python@2' # Python 2 install fails on the link step, failing the whole build. Dep of poppler.
brew install poppler --with-qt

# Add path to find qmake (that will handle all other paths)
export PATH="$(brew --prefix qt5)/bin:${PATH}"

set +xue
