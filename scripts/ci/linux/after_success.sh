#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

MXE_BIN=$HOME/mxe/usr/i686-w64-mingw32.shared.posix
WDIR=$HOME/.wine/drive_c/moolticute_build

VERSION="$(get_version .)"

FILENAME=moolticute_setup_$VERSION


# Debian package
echo "Generating changelog for tag ${BUILD_TAG} [${TRAVIS_COMMIT}]"

rm -f debian/changelog

$DOCKER_EXEC "DEBEMAIL=${USER_EMAIL} dch --create --distribution trusty --package \"moolticute\" \
    --newversion ${DEB_VERSION} \"Release ${BUILD_TAG}\""

echo "Building .deb package..."

$DOCKER_EXEC "cp -f README.md debian/README"
$DOCKER_EXEC "dpkg-buildpackage -b -us -uc && mkdir -p build-linux/deb && cp ../*.deb build-linux/deb"

echo "Building AppImage"
$DOCKER_EXEC "scripts/ci/linux/appimage.sh"

# GitHub release
$DOCKER_EXEC \
    "export TRAVIS_REPO_SLUG=${TRAVIS_REPO_SLUG} PROJECT_NAME=${PROJECT_NAME} TRAVIS_OS_NAME=${TRAVIS_OS_NAME} \
    DEB_VERSION=${DEB_VERSION}; \
    source /usr/local/bin/tools.sh; \
    create_github_release_linux ${BUILD_TAG}"

# upload_file build/$FILENAME.exe $(sha256sum build/$FILENAME.exe | cut -d' ' -f1) "windows"
# upload_file build/updater.json $(sha256sum build/updater.json | cut -d' ' -f1) "windows"

popd
