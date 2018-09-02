#!/bin/bash
set -ev

BUILD_DIR="/moolticute/build_win"

# normally should be defined by Dockerfile because the image knows where it is
if [ -n "$MXE_BASE" ] ; then
    echo "Warning: MXE_BASE undefined" >&2
    export MXE_BASE="$HOME/mxe"
fi

export PATH=$MXE_BASE/usr/bin:$PATH

rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

$MXE_BASE/usr/i686-w64-mingw32.shared.posix/qt5/bin/qmake ../Moolticute.pro
make -j$(nproc --all)
