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
    return 0
fi

mkdir -p $WDIR/redist

#Get 3rd party tools
wget_retry https://calaos.fr/mooltipass/tools/windows/mc-agent.exe -O $WDIR/mc-agent.exe
wget_retry https://calaos.fr/mooltipass/tools/windows/mc-cli.exe -O $WDIR/mc-cli.exe
wget_retry https://calaos.fr/download/misc/redist/Win32OpenSSL_Light-1_0_2L.exe -O $WDIR/redist/Win32OpenSSL_Light-1_0_2L.exe
wget_retry https://calaos.fr/download/misc/redist/vcredist_sp1_x86.exe -O $WDIR/redist/vcredist_sp1_x86.exe

for f in $MXE_BIN/bin/libgcc_s_sjlj-1.dll \
         $MXE_BIN/bin/libstdc++-6.dll \
         $MXE_BIN/bin/libwinpthread-1.dll \
         $MXE_BIN/bin/libwebp-5.dll \
         $MXE_BIN/bin/zlib1.dll \
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

#create update manifest
cat > build/updater.json <<EOF
{ "updates": { "windows": { "latest-version": "$VERSION", "download-url": "https://calaos.fr/mooltipass/windows/$FILENAME.exe" }}}
EOF

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

upload_file build/$FILENAME.exe $(sha256sum build/$FILENAME.exe | cut -d' ' -f1) "windows"
upload_file build/updater.json $(sha256sum build/updater.json | cut -d' ' -f1) "windows"

popd
