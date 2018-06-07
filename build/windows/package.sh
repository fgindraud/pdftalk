#!/usr/bin/env bash
set -xue

# Reduce binary size
strip release/pdftalk.exe

cp release/pdftalk.exe pdftalk-windows-${BITS}.exe

set +xue
