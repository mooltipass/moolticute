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

mkdir -p $WDIR

#Get 3rd party tools
curl https://calaos.fr/mooltipass/tools/windows/moolticute_ssh-agent.exe -o $WDIR/moolticute_ssh-agent.exe
curl https://calaos.fr/mooltipass/tools/windows/moolticute-cli.exe -o $WDIR/moolticute-cli.exe

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
         build/release/MoolticuteApp.exe \
         build/release/moolticuted.exe
do
    cp -R $f $WDIR
done

pushd win

echo "#define MyAppVersion \"$VERSION\"" > build.iss
cat installer.iss >> build.iss
chmod +x iscc
./iscc build.iss

#create update manifest
cat > build/updater.json <<EOF
{ "updates": { "windows": { "latest-version": "$VERSION", "download-url": "https://calaos.fr/mooltipass/windows/$FILENAME.exe" }}}
EOF

upload_file build/$FILENAME.exe $(sha256sum build/$FILENAME.exe | cut -d' ' -f1) "windows"
upload_file build/updater.json $(sha256sum build/updater.json | cut -d' ' -f1) "windows"

popd
