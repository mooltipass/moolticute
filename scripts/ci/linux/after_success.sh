#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

MXE_BIN=$HOME/mxe/usr/i686-w64-mingw32.shared
WDIR=$HOME/Moolticute_win32

VERSION="$(get_version .)"

FILENAME=Moolticute_win32_$VERSION

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(cat .git/HEAD)"  ]; then
    echo "Not uploading package"
    return 0
fi

mkdir -p $WDIR

for f in $MXE_BIN/bin/libgcc_s_sjlj-1.dll \
         $MXE_BIN/bin/libstdc++-6.dll \
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

#create a zip
( cd $HOME;
  zip --compression-method deflate -r $FILENAME.zip $(basename $WDIR) )

upload_file $HOME/$FILENAME.zip $(sha256sum $HOME/$FILENAME.zip | cut -d' ' -f1) "windows"
