#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

VERSION="$(get_version .)"

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(git rev-parse HEAD)"  ]; then
    echo "Not uploading package"
    exit 0
fi

# Debian package
docker exec mc-deb bash /scripts/build_source.sh $VERSION xenial
docker exec mc-deb bash /scripts/build_source.sh $VERSION bionic
docker exec mc-deb bash /scripts/build_source.sh $VERSION focal
docker exec mc-deb bash /scripts/build_source.sh $VERSION jammy
docker exec mc-deb bash /scripts/build_source.sh $VERSION lunar 
docker exec mc-deb bash /scripts/build_source.sh $VERSION noble 

#windows and appimage
docker exec appimgbuilder bash /scripts/package.sh
# windows builds: not using digital certificate anymore
#docker exec winbuilder bash -c "export CODESIGN_WIN_PASS=${CODESIGN_WIN_PASS}; /scripts/package.sh"
docker exec winbuilder bash -c "/scripts/package.sh"

#prepare files to upload volume
mkdir -p $HOME/uploads

# windows builds: do not upload .exe as signing is done after zip upload
#cp -v \
#    packages/*.{exe,zip} \
#    build-appimage/*.AppImage \
#    $HOME/uploads/
cp -v \
    packages/*.zip \
    build-appimage/*.AppImage \
    $HOME/uploads/

EXE_FILE="$(ls packages/*.exe 2> /dev/null | head -n 1)"
APPIMAGE_FILE=$(find $HOME/uploads -iname '*.AppImage')

#create json (only used by testing builds)
cat > $HOME/uploads/updater.json <<EOF
[{
    "tag_name": "$VERSION",
    "html_url": "https://mooltipass-tests.com/mc_betas/$VERSION",
    "body": "",
    "assets": [
        {
            "name": "$(basename $EXE_FILE)",
            "browser_download_url": "https://mooltipass-tests.com/mc_betas/$VERSION/$(basename $EXE_FILE)"
        },
        {
            "name": "$(basename $APPIMAGE_FILE)",
            "browser_download_url": "https://mooltipass-tests.com/mc_betas/$VERSION/$(basename $APPIMAGE_FILE)"
        }
    ]
}]
EOF

docker run -t --name mc-upload -d \
    -v $HOME/uploads:/uploads \
	-e "GITHUB_LOGIN=${GITHUB_LOGIN}" \
	-e "GITHUB_TOKEN=${GITHUB_TOKEN}" \
    mooltipass/mc-upload

for f in $HOME/uploads/*
do
    ff=$(basename $f)
    echo uploading $ff
    if [ -f $HOME/uploads/$ff ]
    then
        docker exec mc-upload bash -c "export SFTP_USER=${MC_BETA_UPLOAD_SFTP_USER} SFTP_PASS=${MC_BETA_UPLOAD_SFTP_PASS} ; /scripts/upload.sh $VERSION $ff"
    fi
done

# Trigger new build on OBS
# Requires the following list of environment variables set
# - OBS_API, for example "https://api.opensuse.org"
# - OBS_PROJ, for example "home:someuser:someproject"
# - OBS_USER, the user to authenticate as against the OBS_API
# - OBS_PASS, the user's password

# Just to be clear. The revision *must* be a git tag matching the python regex
# v([0-9\.]*)(-testing)?(.*)
REVISION="$VERSION"
OBS_COMMIT_MSG="Update package to revision ${REVISION}"

OBS_PKG="moolticute"
if endsWith -testing "$VERSION"; then
    OBS_PKG="moolticute-testing"
fi

# Fetch current _service file to CWD
curl -u $OBS_USER:$OBS_PASS -X GET ${OBS_API}/source/${OBS_PROJ}/${OBS_PKG}/_service -o _service

# Update revision to build
sed -i "s|<param name=\"revision\">\(.*\)</param>|<param name=\"revision\">${REVISION}</param>|" _service

# Push modified _service file back to OBS
curl -u $OBS_USER:$OBS_PASS -X PUT -T _service ${OBS_API}/source/${OBS_PROJ}/${OBS_PKG}/_service

# Commit changes, which will trigger a new build and eventually release
curl -u $OBS_USER:$OBS_PASS -X POST ${OBS_API}/source/${OBS_PROJ}/${OBS_PKG}?cmd=commit --data-urlencode "comment=${OBS_COMMIT_MSG}"
