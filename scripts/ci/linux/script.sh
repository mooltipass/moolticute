#!/bin/bash
set -ev

mkdir build
pushd build

$HOME/mxe/usr/i686-w64-mingw32.shared/qt5/bin/qmake ../Moolticute.pro
make

popd
