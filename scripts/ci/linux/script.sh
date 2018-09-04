#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

make_version .

docker_exec_in "$CONTAINER_WIN_NAME" \
    "cd moolticute ; ./scripts/build/build-win.sh"

#docker_exec_in "$CONTAINER_DEB_NAME" \
    "cd moolticute ; ./scripts/build/build-deb.sh" &

#docker_exec_in "$CONTAINER_APPIMAGE_NAME" \
    "cd moolticute ; ./scripts/build/build-appimage.sh" &

#wait
