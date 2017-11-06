#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

make_version .

# Linux build
echo $DOCKER_EXEC
$DOCKER_EXEC "mkdir -p /app/build-linux || true && cd /app/build-linux && qmake /app/Moolticute.pro && make"

# Windows build
# mkdir build
# pushd build

#Cleaning env make travis failed in some case
#unset `env | \
#grep -vi '^EDITOR=\|^HOME=\|^LANG=\|MXE\|^PATH=' | \
#grep -vi 'PKG_CONFIG\|PROXY\|^PS1=\|^TERM=\|^TRAVIS_OS_NAME=\|^UPLOAD_KEY=' | \
#   cut -d '=' -f1 | tr '\n' ' '`

export PATH=$HOME/mxe/usr/bin:$PATH
export MXE_BASE=$HOME/mxe

# $MXE_BASE/usr/i686-w64-mingw32.shared.posix/qt5/bin/qmake ../Moolticute.pro
# make

popd
