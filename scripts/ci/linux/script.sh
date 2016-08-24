#!/bin/bash
set -ev

mkdir build
pushd build

unset `env | \
grep -vi '^EDITOR=\|^HOME=\|^LANG=\|MXE\|^PATH=' | \
grep -vi 'PKG_CONFIG\|PROXY\|^PS1=\|^TERM=\|^TRAVIS_OS_NAME=' | \
   cut -d '=' -f1 | tr '\n' ' '`

export PATH=$HOME/mxe/usr/bin:$PATH
export MXE_BASE=$HOME/mxe


$MXE_BASE/usr/i686-w64-mingw32.shared/qt5/bin/qmake ../Moolticute.pro
make

popd
