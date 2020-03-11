#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

mkdir build
pushd build

QTDIR="/usr/local/opt/qt5"
PATH="$QTDIR/bin:$PATH"
LDFLAGS=-L$QTDIR/lib
CPPFLAGS=-I$QTDIR/include

make_version .. macos

qmake ../Moolticute.pro

# Compund exit codes of make and the tests.
make && ./tests/tests

popd
