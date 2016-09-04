#!/usr/bin/env bash
set -xue

# Reduce binary size
strip release/pdftalk.exe
upx -9 release/pdftalk.exe || true

cp release/pdftalk.exe pdftalk-windows-${BITS}.exe

set +xue
