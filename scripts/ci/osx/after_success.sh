#!/bin/bash

#
# creating the Moolticute.dmg with Applications link
#

QTDIR="/usr/local/opt/qt5"
APP=MoolticuteApp
# this directory name will also be shown in the title when the DMG is mounted
TEMPDIR=build/$APP
SIGNATURE="Raoul Hecky"
NAME=`uname`

if [ "$NAME" != "Darwin" ]; then
    echo "This is not a Mac"
    exit 1
fi

echo "Changing bundle identifier"
sed -i -e 's/com.yourcompany.Moolticute/com.Mooltipass.Moolticute/g' build/$APP.app/Contents/Info.plist
# removing backup plist
rm -f build/$APP.app/Contents/Info.plist-e

# copy translation files to app
# cp languages/*.qm build/$APP.app/Contents/Resources

echo "Adding keys"
# add the keys for OSX code signing
security create-keychain -p travis osx-build.keychain
#security import ../travis/osx/apple.cer -k ~/Library/Keychains/osx-build.keychain -T /usr/bin/codesign
security import scripts/ci/osx/developerID_application.cer -k ~/Library/Keychains/osx-build.keychain -T /usr/bin/codesign
#security import ../travis/osx/dist.p12 -k ~/Library/Keychains/osx-build.keychain -P $KEY_PASSWORD -T /usr/bin/codesign
security default-keychain -s osx-build.keychain
security unlock-keychain -p travis osx-build.keychain

# use macdeployqt to deploy the application
#echo "Calling macdeployqt and code signing application"
#$QTDIR/bin/macdeployqt ./$APP.app -codesign="$DEVELOPER_NAME"
echo "Calling macdeployqt"
$QTDIR/bin/macdeployqt build/$APP.app
if [ "$?" -ne "0" ]; then
    echo "Failed to run macdeployqt"
    # remove keys
    security delete-keychain osx-build.keychain 
    exit 1
fi

#echo "Sign the code"
##This signs the code
echo "Sign Code with $SIGNATURE"
codesign --force --verify --verbose --sign "Developer ID Application: $SIGNATURE" build/$APP.app
#codesign -s "$SIGNATURE" -f build/$APP.app
if [ "$?" -ne "0" ]; then
    echo "Failed to sign app bundle"
    exit 1
fi



echo "Verifying code signed app"
codesign --verify --verbose=4 build/$APP.app
spctl --assess --verbose=4 --raw build/$APP.app

echo "Create $TEMPDIR"
#Create a temporary directory if one doesn't exist
mkdir -p $TEMPDIR
if [ "$?" -ne "0" ]; then
    echo "Failed to create temporary folder"
    exit 1
fi

echo "Clean $TEMPDIR"
#Delete the contents of any previous builds
rm -Rf ./$TEMPDIR/*
if [ "$?" -ne "0" ]; then
    echo "Failed to clean temporary folder"
    exit 1
fi

echo "Move application bundle"
#Move the application to the temporary directory
mv build/$APP.app ./$TEMPDIR
if [ "$?" -ne "0" ]; then
    echo "Failed to move application bundle"
    exit 1
fi

echo "Create symbolic link"
#Create a symbolic link to the applications folder
ln -s /Applications ./$TEMPDIR/Applications
if [ "$?" -ne "0" ]; then
    echo "Failed to create link to /Applications"
    exit 1
fi

echo "Create new disk image"
#Create the disk image
rm -f build/$APP.dmg
hdiutil create -srcfolder ./$TEMPDIR -format UDBZ build/$APP.dmg
if [ "$?" -ne "0" ]; then
    echo "Failed to create disk image"
    exit 1
fi

#echo "Code signing disk image"
codesign --force --verify --verbose --sign "$DEVELOPER_NAME" build/$APP.dmg

echo "Verifying code signed disk image"
codesign --verify --verbose=4 build/$APP.dmg
spctl --assess --verbose=4 --raw build/$APP.dmg

echo "moving $APP.dmg to $APP-$VERSION_NUMBER.dmg"
mv build/$APP.dmg build/$APP-$VERSION_NUMBER.dmg

echo "Removing keys"
# remove keys
security delete-keychain osx-build.keychain 

# delete the temporary directory
rm -Rf ./$TEMPDIR/*
if [ "$?" -ne "0" ]; then
    echo "Failed to clean temporary folder"
    exit 1
fi

exit 0
