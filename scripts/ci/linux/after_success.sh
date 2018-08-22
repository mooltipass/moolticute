#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

MXE_BIN=$HOME/mxe/usr/i686-w64-mingw32.shared.posix
WDIR=$HOME/.wine/drive_c/moolticute_build

VERSION="$(get_version .)"

FILENAME=moolticute_setup_$VERSION

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(cat .git/HEAD)"  ]; then
    echo "Not uploading package"
    exit 0
fi

mkdir -p $WDIR/redist

#Get 3rd party tools
wget_retry https://calaos.fr/mooltipass/tools/windows/mc-agent.exe -O $WDIR/mc-agent.exe
wget_retry https://calaos.fr/mooltipass/tools/windows/mc-cli.exe -O $WDIR/mc-cli.exe

for f in $MXE_BIN/bin/libgcc_s_sjlj-1.dll \
         $MXE_BIN/bin/libstdc++-6.dll \
         $MXE_BIN/bin/libwinpthread-1.dll \
         $MXE_BIN/bin/libwebp-5.dll \
         $MXE_BIN/bin/zlib1.dll \
         $MXE_BIN/bin/ssleay32.dll \
         $MXE_BIN/bin/libeay32.dll \
         $MXE_BIN/bin/icudt56.dll \
         $MXE_BIN/bin/icuin56.dll \
         $MXE_BIN/bin/icuuc56.dll \
         $MXE_BIN/qt5/bin/Qt5Core.dll \
         $MXE_BIN/qt5/bin/Qt5Gui.dll \
         $MXE_BIN/qt5/bin/Qt5Network.dll \
         $MXE_BIN/qt5/bin/Qt5Widgets.dll \
         $MXE_BIN/qt5/bin/Qt5WebSockets.dll \
         $MXE_BIN/qt5/plugins/imageformats \
         $MXE_BIN/qt5/plugins/platforms \
         build/release/moolticute.exe \
         build/release/moolticuted.exe
do
    cp -R $f $WDIR
done

find_and_sign $WDIR

pushd win

echo "#define MyAppVersion \"$VERSION\"" > build.iss
cat installer.iss >> build.iss
chmod +x iscc
./iscc build.iss

sign_binary build/$FILENAME.exe

#Create a portable zip for windows
ZIPFILE=moolticute_portable_win32_${VERSION}.zip
pushd $WDIR/..
mv moolticute_build moolticute_$VERSION
WDIR=$(pwd)
zip --compression-method deflate -r $ZIPFILE moolticute_$VERSION
popd

mv $WDIR/$ZIPFILE build/

# Debian package
echo "Generating changelog for tag ${BUILD_TAG} [${TRAVIS_COMMIT}]"

$DOCKER_EXEC "git clone https://github.com/mooltipass/mooltipass-udev"

rm -f debian/changelog

$DOCKER_EXEC "DEBEMAIL=${USER_EMAIL} dch --create --distribution trusty --package \"moolticute\" \
    --newversion ${DEB_VERSION} \"Release ${BUILD_TAG}\""

echo "Building .deb package..."

$DOCKER_EXEC \
    "mkdir -p build-linux && \
    wget https://calaos.fr/mooltipass/tools/linux/mc-agent -O build-linux/mc-agent && \
    wget https://calaos.fr/mooltipass/tools/linux/mc-agent -O build-linux/mc-cli && \
    chmod +x build-linux/mc-agent && \
    chmod +x build-linux/mc-cli"

$DOCKER_EXEC \
    "dpkg-buildpackage -b -us -uc && \
    mkdir -p build-linux/deb && \
    cp ../*.deb build-linux/deb"

echo "Building AppImage"
$DOCKER_EXEC "scripts/ci/linux/appimage.sh"

#Check if this is a test release or not
if endsWith -testing "$VERSION" ; then

    $DOCKER_EXEC \
        "export TRAVIS_REPO_SLUG=${TRAVIS_REPO_SLUG} PROJECT_NAME=${PROJECT_NAME} TRAVIS_OS_NAME=${TRAVIS_OS_NAME} \
        DEB_VERSION=${DEB_VERSION} SFTP_USER=${MC_BETA_UPLOAD_SFTP_USER} SFTP_PASS=${MC_BETA_UPLOAD_SFTP_PASS}; \
        source /usr/local/bin/tools.sh; \
        create_beta_release_linux ${BUILD_TAG}"

else

    # GitHub release
    $DOCKER_EXEC \
        "export TRAVIS_REPO_SLUG=${TRAVIS_REPO_SLUG} PROJECT_NAME=${PROJECT_NAME} TRAVIS_OS_NAME=${TRAVIS_OS_NAME} \
        DEB_VERSION=${DEB_VERSION}; \
        source /usr/local/bin/tools.sh; \
        create_github_release_linux ${BUILD_TAG}"

fi

popd
