#!/bin/bash

if [ $# -ne 1 ]
then
    echo "Usage: $0 <version>"
    exit 1
fi

VERSION=$1

echo "Copy files..."
cp Info.plist ../../build/Moolticute.app/Contents/
cp icon.icns ../../build/Moolticute.app/Contents/Resources/

echo "Creating dmg package..."
macdeployqt ../../build/Moolticute.app -dmg
mv ../../build/calaos_installer.dmg ../../Moolticute_macosx_$VERSION.dmg

echo "Done."
echo "Package: ../../Moolticute_macosx_$VERSION.dmg"
