#!/bin/bash

########################################################################
# Package the binaries built on Travis-CI as an AppImage
# By Simon Peter 2016
# Modified for Moolticute by Alexis López 2017
# For more information, see http://appimage.org/
########################################################################


export ARCH=$(arch)

APP=moolticute
LOWERAPP=${APP,,}

BASE_PATH=$PWD/build-appimage/
APPDIR=$BASE_PATH/$APP/$APP.AppDir/
TARGET_DIR=$APPDIR/usr/

mkdir -p $TARGET_DIR

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/appimage_functions.sh
source $SCRIPTDIR/../funcs.sh

########################################################################
# Install application binaries to the AppDir
########################################################################
cp ./debian/moolticute/usr/* $TARGET_DIR -r

cp ./data/moolticute.desktop $BASE_PATH/$APP/$APP.AppDir
cp ./img/AppIcon.svg $BASE_PATH/$APP/$APP.AppDir/moolticute.svg
cp ./data/moolticute.sh $BASE_PATH/$APP/$APP.AppDir/usr/bin/

# Use the wrapper instead of the 
sed -i 's/Exec=\/usr\/bin\/moolticute/Exec=moolticute.sh/g' /$BASE_PATH/$APP/$APP.AppDir/moolticute.desktop 

########################################################################
# Copy desktop and icon file to AppDir for AppRun to pick them up
########################################################################
cd $BASE_PATH/$APP/$APP.AppDir

get_apprun
# get_desktop
# get_icon

cd ..
########################################################################
# Other appliaction-specific finishing touches
########################################################################

# Bundle Qt and all the plugins needed

generate_status

echo "deb http://archive.ubuntu.com/ubuntu/ xenial main restricted universe multiverse" > sources.list
apt-get $OPTIONS update

cd ./$APP.AppDir/


apt-get install -y \
    fuse \
    libfuse-dev \
    libfuse2 \
    libc6 \
    libgcc1 \
    libqt5core5a \
    libqt5gui5 \
    libqt5network5 \
    libqt5websockets5 \
    libqt5widgets5 \
    libstdc++6 \
    libusb-1.0-0 \
    libssl1.0.0

apt-get install -f -y

########################################################################
# Copy in the dependencies that cannot be assumed to be available
# on all target systems
########################################################################

# copy all qt platform plugins 
mkdir -p $TARGET_DIR/lib/x86_64-linux-gnu/qt5/plugins/platforms/
cp -r /usr/lib/x86_64-linux-gnu/qt5/plugins/platforms/* $TARGET_DIR/lib/x86_64-linux-gnu/qt5/plugins/platforms/

#Copy openssl

#TOFIX: Actually copying openssl is not working. SSL is going to try to load root certificates from the system
# and the path differs from all distro...
# More information here:
# https://gitlab.com/probono/platformissues#certificates

#mkdir -p $APPDIR/lib/x86_64-linux-gnu/
#cp /lib/x86_64-linux-gnu/libcrypto* /lib/x86_64-linux-gnu/libssl* $APPDIR/lib/x86_64-linux-gnu/
#cp -r /usr/lib/x86_64-linux-gnu/openssl* $TARGET_DIR/lib/x86_64-linux-gnu/

# copy deps recursively 
old_list=""
list=$(find .)
while [ "$old_list" != "$list" ]
do
   copy_deps
   old_list="$list"
   list=$(find . | sort | uniq)
done



########################################################################
# Delete stuff that should not go into the AppImage
########################################################################

# Delete dangerous libraries; see
# https://github.com/probonopd/AppImages/blob/master/excludelist
delete_blacklisted

########################################################################
# desktopintegration asks the user on first run to install a menu item
########################################################################

get_desktopintegration $LOWERAPP
patch -p0 ./usr/bin/moolticute.sh.wrapper < $SCRIPTDIR/moolticute.sh.wrapper.patch

########################################################################
# Determine the version of the app; also include needed glibc version
########################################################################

echo "OLD VERSION $VERSION"
VERSION=$(get_version .)
echo "new version $VERSION"

########################################################################
# Patch away absolute paths; it would be nice if they were relative
########################################################################

#Prevent our moolticute.sh and moolticute.sh.wrapper to be patched
find usr/ -type f ! -path "usr/bin/moolticute.sh*" -exec sed -i -e 's|/usr|././|g' {} \;
find usr/ -type f -exec sed -i -e 's@././/bin/env@/usr/bin/env@g' {} \;

########################################################################
# AppDir complete
# Now packaging it as an AppImage
########################################################################

cd .. # Go out of AppImage

generate_type2_appimage

########################################################################
# Upload the AppDir
########################################################################

# curl --upload-file *.AppImage https://transfer.sh/Moolticute.AppImage
# echo "AppImage has been uploaded to the URL above; use something like GitHub Releases for permanent storage"
