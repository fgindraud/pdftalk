#!/usr/bin/env bash
set -xue

# Reduce binary size
strip pdftalk

cp pdftalk pdftalk-linux-$(uname -m)

set +xue
