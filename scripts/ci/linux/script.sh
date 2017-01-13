#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

mkdir build
pushd build

unset `env | \
grep -vi '^EDITOR=\|^HOME=\|^LANG=\|MXE\|^PATH=' | \
grep -vi 'PKG_CONFIG\|PROXY\|^PS1=\|^TERM=\|^TRAVIS_OS_NAME=\|^UPLOAD_KEY=' | \
   cut -d '=' -f1 | tr '\n' ' '`

export PATH=$HOME/mxe/usr/bin:$PATH
export MXE_BASE=$HOME/mxe

make_version ..

$MXE_BASE/usr/i686-w64-mingw32.shared.posix/qt5/bin/qmake ../Moolticute.pro
make

popd
