#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

#copy source for debian package
make_version . deb
wget_retry https://github.com/mooltipass/mooltipass-udev/raw/master/udev/69-mooltipass.rules -O 69-mooltipass.rules
cp -R . $HOME/build-debs/moolticute-${DEB_VERSION}

# Linux build
make_version . appimage
docker exec appimgbuilder /bin/bash /scripts/build.sh

# Windows build
make_version . windows
docker exec winbuilder /bin/bash /scripts/build.sh

