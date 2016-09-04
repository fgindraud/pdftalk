#!/usr/bin/env bash
set -xue

# Pack binary
strip pdftalk
upx -9 pdftalk || true

cp pdftalk pdftalk-linux-$(uname -m)

set +xue
