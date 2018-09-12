#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

make_version .

#copy source for debian package
cp -R . $HOME/build-debs/moolticute-${DEB_VERSION}

# Linux build
docker exec appimgbuilder /bin/bash /scripts/build.sh

# Windows build
docker exec winbuilder /bin/bash /scripts/build.sh

