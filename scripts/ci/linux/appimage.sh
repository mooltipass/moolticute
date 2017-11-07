#!/bin/bash

########################################################################
# Package the binaries built on Travis-CI as an AppImage
# By Simon Peter 2016
# Modified for Moolticute by Alexis López 2017
# For more information, see http://appimage.org/
########################################################################

export ARCH=$(arch)

BASE_PATH=$(pwd)/appimage_build/

APP=moolticute
LOWERAPP=${APP,,}

BINARIES_DIR=$BASE_PATH/$APP/$APP.AppDir/usr/
mkdir -p $BINARIES_DIR

source ./appimage_functions.sh

cd $BASE_PATH/$APP/$APP.AppDir

echo "[Desktop Entry]
Version=1.0
Type=Application
Name=Moolticute
Comment=Moolticute allows your browser extensions to talk with the Mooltipass.
TryExec=moolticute
Exec=moolticute %F
Icon=moolticute" > moolticute.desktop

wget -O moolticute.svg https://raw.githubusercontent.com/mooltipass/moolticute/master/img/AppIcon.svg

########################################################################
# Copy desktop and icon file to AppDir for AppRun to pick them up
########################################################################

get_apprun
get_desktop
get_icon

########################################################################
# Other appliaction-specific finishing touches
########################################################################

# Bundle Qt and all the plugins needed

cd ..

generate_status

echo "deb http://archive.ubuntu.com/ubuntu/ xenial main restricted universe multiverse" > sources.list
apt-get $OPTIONS update

cd ./$APP.AppDir/

find $BASE_PATH -iname '*.deb' -exec dpkg -x {} . \; || true
apt-get install \
    libc6 \
    libgcc1 \
    libqt5core5a \
    libqt5gui5 \
    libqt5gui5-gles \
    libqt5network5 \
    libqt5websockets5 \
    libqt5widgets5 \
    libstdc++6 \
    libusb-1.0-0

apt-get install -f -y

########################################################################
# Copy in the dependencies that cannot be assumed to be available
# on all target systems
########################################################################

# copy all qt platform plugins 
mkdir -p ./usr/lib/x86_64-linux-gnu/qt5/plugins/platforms/
cp -r /usr/lib/x86_64-linux-gnu/qt5/plugins/platforms/* ./usr/lib/x86_64-linux-gnu/qt5/plugins/platforms/

list=$(find $BINARIES_DIR -type f)
for bynary in $list; do
    if [ -f $bynary ]; then
        deps=$(ldd $bynary  | cut -d ' ' -f 3)
        for file in $deps; do
            if [ -f $file ]; then
                cp $file .$file
            fi
        done
    fi;
done


copy_deps

########################################################################
# Delete stuff that should not go into the AppImage
########################################################################

# Delete dangerous libraries; see
# https://github.com/probonopd/AppImages/blob/master/excludelist
delete_blacklisted
find . -name *harfbuzz* -delete

########################################################################
# desktopintegration asks the user on first run to install a menu item
########################################################################

get_desktopintegration $LOWERAPP

########################################################################
# Determine the version of the app; also include needed glibc version
########################################################################

VERSION=$(echo "$VERSION" | sed -e 's/alpha+gitexport/git/')

########################################################################
# Patch away absolute paths; it would be nice if they were relative
########################################################################

find usr/ -type f -exec sed -i -e 's|/usr|././|g' {} \;
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

transfer ../*.AppImage
echo "AppImage has been uploaded to the URL above; use something like GitHub Releases for permanent storage"
