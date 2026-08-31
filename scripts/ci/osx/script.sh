#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/env.sh

mkdir build
pushd build

detect_qtdir
QMAKE_BIN="$(qmake_bin)"
PATH="$QTDIR/bin:$PATH"
LDFLAGS=-L$QTDIR/lib
CPPFLAGS=-I$QTDIR/include

make_version .. macos

"$QMAKE_BIN" ../Moolticute.pro

# Compund exit codes of make and the tests.
make && ./tests/tests

popd
