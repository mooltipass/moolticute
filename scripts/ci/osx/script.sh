#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

mkdir build
pushd build

QTDIR="/opt/Qt/6.2.4/macos"
PATH="$QTDIR/bin:$PATH"
LDFLAGS=-L$QTDIR/lib
CPPFLAGS=-I$QTDIR/include

make_version .. macos

qmake ../Moolticute.pro

# Compund exit codes of make and the tests.
make && ./tests/tests

popd
