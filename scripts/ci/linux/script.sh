#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

make_version .

# Linux build
echo $DOCKER_EXEC
$DOCKER_EXEC "mkdir -p /app/build-linux || true && cd /app/build-linux && qmake /app/Moolticute.pro && make"

