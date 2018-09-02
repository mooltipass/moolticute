#!/bin/bash
set -ev

BASEDIR="/moolticute"

mkdir -p $BASEDIR/build-linux
cd $BASEDIR/build-linux
qmake $BASEDIR/Moolticute.pro
make
./tests/tests
