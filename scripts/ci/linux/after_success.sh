#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

VERSION="$(get_version .)"

#Only build if the commit we are building is for the last tag
if [ "$(git rev-list -n 1 $VERSION)" != "$(cat .git/HEAD)"  ]; then
    echo "Not uploading package"
    exit 0
fi

# Debian package
docker exec mc-deb bash /scripts/build_source.sh $VERSION xenial
docker exec mc-deb bash /scripts/build_source.sh $VERSION bionic
docker exec mc-deb bash /scripts/build_source.sh $VERSION disco

#windows and appimage
docker exec appimgbuilder bash /scripts/package.sh
docker exec winbuilder bash -c "export CODESIGN_WIN_PASS=${CODESIGN_WIN_PASS}; /scripts/package.sh"

#prepare files to upload volume
mkdir -p $HOME/uploads
cp -v \
    packages/*.{exe,zip} \
    build-appimage/*.AppImage \
    $HOME/uploads/

EXE_FILE="$(ls $HOME/uploads/*.exe 2> /dev/null | head -n 1)"
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

