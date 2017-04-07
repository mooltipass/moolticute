#!/bin/bash

set -ev

#
# creating the Moolticute.dmg with Applications link
#

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

VERSION="$(get_version .)"

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(cat .git/HEAD)"  ]; then
    echo "Not uploading package"
    return 0
fi

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

cat build/$APP.app/Contents/Info.plist

echo "Changing bundle identifier"
sed -i -e 's/com.yourcompany.Moolticute/com.Mooltipass.Moolticute/g' build/$APP.app/Contents/Info.plist
# removing backup plist
rm -f build/$APP.app/Contents/Info.plist-e

# Copy daemon to bundle
cp build/moolticuted build/$APP.app/Contents/MacOS/

#Get 3rd party tools
curl https://calaos.fr/mooltipass/tools/macos/moolticute_ssh-agent -o build/$APP.app/Contents/MacOS/moolticute_ssh-agent
curl https://calaos.fr/mooltipass/tools/macos/moolticute-cli -o build/$APP.app/Contents/MacOS/moolticute-cli

# use macdeployqt to deploy the application
#echo "Calling macdeployqt and code signing application"
#$QTDIR/bin/macdeployqt ./$APP.app -codesign="$DEVELOPER_NAME"
echo "Calling macdeployqt"
$QTDIR/bin/macdeployqt build/$APP.app
if [ "$?" -ne "0" ]; then
    echo "Failed to run macdeployqt"
    # remove keys
 #   security delete-keychain osx-build.keychain 
    exit 1
fi

#Call fix to change all rpath
wget https://raw.githubusercontent.com/aurelien-rainone/macdeployqtfix/master/macdeployqtfix.py
python macdeployqtfix.py build/$APP.app/Contents/MacOS/MoolticuteApp /usr/local/Cellar/qt5/5.*/
python macdeployqtfix.py build/$APP.app/Contents/MacOS/moolticuted /usr/local/Cellar/qt5/5.*/

#install appdmg https://github.com/LinusU/node-appdmg a tool to create awesome dmg !
npm install -g appdmg
appdmg mac/appdmg.json build/$APP-$VERSION.dmg

#create update manifest
cat > build/updater.json <<EOF
{ "updates": { "osx": { "latest-version": "$VERSION", "download-url": "https://calaos.fr/mooltipass/macos/$APP-$VERSION.dmg" }}}
EOF

upload_file build/$APP-$VERSION.dmg $(shasum -a 256 build/$APP-$VERSION.dmg | cut -d' ' -f1) "macos"
upload_file build/updater.json $(shasum -a 256 build/updater.json | cut -d' ' -f1) "macos"

exit 0

