#!/bin/bash

set -e

export PROJECT_NAME=moolticute

export TRAVIS_BUILD_DIR=$(pwd)

export BUILD_TAG=$(git tag --points-at=HEAD --sort version:refname | head -n 1)

export DOCKER_IMAGE_WIN_NAME="mc-win-builder"
export DOCKER_IMAGE_DEB_NAME="mc-deb-builder"
export DOCKER_IMAGE_APPIMAGE_NAME="mc-appimage-builder"

export CONTAINER_WIN_NAME="win-builder"
export CONTAINER_DEB_NAME="deb-builder"
export CONTAINER_APPIMAGE_NAME="appimage-builder"

export DOCKER_EXEC_ENV=

export DOCKER_COMPOSE_CONFIG=" -f docker-compose.base.yml"

if [ "${ENV}" == "dev" ]; then
    DOCKER_COMPOSE_CONFIG+=" -f docker-compose.dev.yml"
else
    DOCKER_COMPOSE_CONFIG+=" -f docker-compose.yml"
fi

export BUILD_DIR="${TRAVIS_BUILD_DIR}/build-linux"
export DEB_VERSION=$(echo ${BUILD_TAG} | tr 'v' ' ' | xargs)

export USER_EMAIL="limpkin@limpkin.fr"

export GITHUB_REPO=$(echo "${TRAVIS_REPO_SLUG}" | rev | cut -d "/" -f1 | rev)
export GITHUB_ACCOUNT=$(echo "${TRAVIS_REPO_SLUG}" | rev | cut -d "/" -f2 | rev)


# Execute a command in one of docker container
# Args:
#  $1 - container name to execute in
#  $2 ....   - the command and arguments to execute
function docker_exec_in()
{
    local container_name="$1"
    shift

    docker exec ${DOCKER_EXEC_ENV} ${container_name} /bin/bash -c "$@"
}

#Usage: get_version /path/to/repo
function get_version()
{
    repo=$1
    pushd $repo > /dev/null
    git describe --tags --abbrev=0
    popd > /dev/null
}

#Usage: make_version /path/to/repo
function make_version()
{
    VERSION="$(get_version $1)"
    echo "#ifndef VERSION__H" > $1/src/version.h
    echo "#define VERSION__H" >> $1/src/version.h
    echo "#define APP_VERSION \"$VERSION\"" >> $1/src/version.h
    if endsWith -testing "$VERSION"; then
        echo "#define APP_RELEASE_TESTING 1" >> $1/src/version.h
    fi
    echo "#endif" >> $1/src/version.h
}

function wget_retry()
{
    count=0
    while [ $count -le 4 ]; do
        echo Downloading $@
        set +e
        wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 0 --continue $@
        ret=$?
        set -e
        if [ $ret = 0 ]
        then
            return 0
        fi
        sleep 1
        count=$((count+1))
        echo Download failed.
    done
    return 1
}

function upload_file()
{
    FNAME=$1
    HASH=$2
    INSTALLPATH=$3

    curl -X POST \
        -H "Content-Type: multipart/form-data" \
        -F "upload_key=$UPLOAD_KEY" \
        -F "upload_folder=$INSTALLPATH" \
        -F "upload_sha256=$HASH" \
        -F "upload_file=@$FNAME" \
        -F "upload_replace=true" \
        https://calaos.fr/mooltipass/upload
}

# Upload a release asset specifying a MIME type
#
# Note, this command requires `jq` to find the release `upload_url`.
#
# Usage:
#
#     upload_asset username reponame 1087938 \
#         foo.tar application/x-tar < foo.tar
#
# * (stdin)
#   The contents of the file to upload.
#
# Positional arguments
#
function upload_asset_mime()
{
    local owner="${1:?Owner name required.}"
    #   A GitHub user or organization.
    local repo="${2:?Repo name required.}"
    #   A GitHub repository.
    local release_id="${3:?Release ID required.}"
    #   The unique ID of the release; see list_releases.
    local name="${4:?File name is required.}"
    #   The file name of the asset.
    local mime="${5:?MIME type is required.}"
    #
    # Keyword arguments
    #
    local _filter='"\(.state)\t\(.browser_download_url)"'
    #   A jq filter to apply to the return data.
    shift 5
    ok.sh _opts_filter "$@"
    local upload_url=$(ok.sh release "$owner" "$repo" "$release_id" _filter="(.upload_url)" | sed -e 's/{?name,label}/?name='"$name"'/g')
    : "${upload_url:?Upload URL could not be retrieved.}"
    ok.sh _post "$upload_url" filename="$name" mime_type="$mime" > /dev/null
}

# Get a release ID specifying its name
#
# Usage:
#
#     get_release_id_by_name username reponame
#
# Positional arguments
#
function get_release_id_by_name()
{
    local NAME="${1:?Release name required.}"

    ok.sh list_releases "$GITHUB_ACCOUNT" "$GITHUB_REPO" _filter='.[] | "\(.name)\t\(.id)"' | grep "$NAME[^-]" | awk '{ print $2 }'
}

# Get a release asset ID specifying its name
#
# Usage:
#
#     get_release_asset_id_by_name releaseid assetname
#
# Positional arguments
#
function get_release_asset_id_by_name()
{
    local RELEASE_ID="${1:?Release ID name required.}"
    local NAME="${2:?Release asset name required.}"

    ok.sh release_assets "$GITHUB_ACCOUNT" "$GITHUB_REPO" "$RELEASE_ID" _filter='.[] | "\(.name)\t\(.id)"' | grep "$NAME[^-]" | awk '{ print $2 }'
}

# Delete a release asset ID by its ID
#
# Usage:
#
#     delete_release_asset_by_id assetid
#
# Positional arguments
#
function delete_release_asset_by_id()
{
    local ASSET_ID="${1:?Asset ID required.}"
    local delete_url="/repos/${GITHUB_ACCOUNT}/${GITHUB_REPO}/releases/assets/${ASSET_ID}"

    ok.sh _delete "$delete_url"
}

# Create a release and upload a new assset to that release
#
# Usage:
#
#     create_release_and_upload_asset reponame filepath
#
# Positional arguments
#
function create_release_and_upload_asset()
{
    local TAG=$1
    local FILE_PATH=$2

    if [ ! -f "$FILE_PATH" ]; then
       echo "The file $FILE_PATH does not exist so it can't be uploaded"
       return 0
    fi

    local MIME_TYPE=$(file --mime-type $FILE_PATH)
    local FILE_NAME=$(basename $FILE_PATH)
    local FILE_DIR=$(dirname $FILE_PATH)

    RELEASE_ID=$(get_release_id_by_name "$TAG")

    if [ -z "$RELEASE_ID" ]; then
        echo "Release for tag $TAG doesn't exist. Creating one..."
        RELEASE_ID=$(ok.sh create_release "$GITHUB_ACCOUNT" "$GITHUB_REPO" "$TAG" name="$TAG" | awk '{print $2}')
    else
        echo "Release for tag $TAG already exists. Using it."
    fi

    echo "Release ID: [${RELEASE_ID}]"

    ASSET_ID=$(get_release_asset_id_by_name "$RELEASE_ID" "$FILE_NAME")

    if [ -z "$ASSET_ID" ]; then
        echo "Asset not yet existing for release $TAG. Creating one..."
    else
        echo "Asset already existing in release $TAG. Removing it and preparing for a fresh upload."

        delete_release_asset_by_id "$ASSET_ID"
    fi

    echo "Uploading new asset $FILE_NAME to release ID $RELEASE_ID..."

    # We need to be in the directory or ok.sh goes crazy - seems it can't properly handle a file :(
    pushd $FILE_DIR > /dev/null
    upload_asset_mime "$GITHUB_ACCOUNT" "$GITHUB_REPO" "$RELEASE_ID" "$FILE_NAME" "$MIME_TYPE" < ${FILE_NAME}
    popd > /dev/null
}

# Create a a GitHub release for the specified version and upload all applicable assets
#
# Usage:
#
#     create_github_release_linux version
#
# Positional arguments
#
function create_github_release_linux()
{
    local VERSION="${1:?Release version required.}"
    local DEB_VERSION=$(echo $VERSION | tr 'v' ' ' | xargs)
    local DEB_NAME="${PROJECT_NAME}_${DEB_VERSION}_amd64.deb"
    local DEB_FILE="build-linux/deb/${DEB_NAME}"
    local APPIMAGE_FILE=$(find build-appimage -iname '*.AppImage')
    local EXE_FILE="$(ls win/build/*.exe 2> /dev/null | head -n 1)"
    local ZIP_FILE="$(ls win/build/*.zip 2> /dev/null | head -n 1)"

    if [ -z "$VERSION" ]; then
        >&2 echo -e "Skipping GitHub release creation (current build does not have a tag)"
        return 0
    fi

    >&2 echo -e "Creating (Linux) GitHub release (tag: $VERSION)"

    create_release_and_upload_asset $VERSION $DEB_FILE
    create_release_and_upload_asset $VERSION $APPIMAGE_FILE
    create_release_and_upload_asset $VERSION $EXE_FILE
    create_release_and_upload_asset $VERSION $ZIP_FILE
}

function create_beta_release_linux()
{
    local VERSION="${1:?Release version required.}"
    local DEB_VERSION=$(echo $VERSION | tr 'v' ' ' | xargs)
    local DEB_NAME="${PROJECT_NAME}_${DEB_VERSION}_amd64.deb"
    local DEB_FILE="build-linux/deb/${DEB_NAME}"
    local APPIMAGE_FILE=$(find build-appimage -iname '*.AppImage')
    local EXE_FILE="$(ls win/build/*.exe 2> /dev/null | head -n 1)"
    local ZIP_FILE="$(ls win/build/*.zip 2> /dev/null | head -n 1)"

    if [ -z "$VERSION" ]; then
        >&2 echo -e "Skipping GitHub release creation (current build does not have a tag)"
        return 0
    fi

    >&2 echo -e "Creating (Linux) beta release (tag: $VERSION)"

    cat > updater.json <<EOF
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
            "name": "$DEB_NAME",
            "browser_download_url": "https://mooltipass-tests.com/mc_betas/$VERSION/$DEB_NAME"
        },
        {
            "name": "$(basename $APPIMAGE_FILE)",
            "browser_download_url": "https://mooltipass-tests.com/mc_betas/$VERSION/$(basename $APPIMAGE_FILE)"
        },
        {
            "name": "$(basename $ZIP_FILE)",
            "browser_download_url": "https://mooltipass-tests.com/mc_betas/$VERSION/$(basename $ZIP_FILE)"
        }
    ]
}]
EOF

    mkdir -p ~/.ssh
    ssh-keyscan -p 54433 -H mooltipass-tests.com >> ~/.ssh/known_hosts

    lftp -p 54433 sftp://${SFTP_USER}:${SFTP_PASS}@mooltipass-tests.com \
        -e "set sftp:auto-confirm yes; \
        cd mc_betas; \
        mkdir -p -f $VERSION; \
        cd $VERSION; \
        put $DEB_FILE; \
        put $APPIMAGE_FILE; \
        put $EXE_FILE; \
        put $ZIP_FILE; \
        cd ..; \
        rm -f updater.json; \
        put updater.json; \
        bye"
}

# Create a a GitHub release for the specified version and upload all applicable assets
#
# Usage:
#
#     create_github_release_osx version
#
# Positional arguments
#
function create_github_release_osx()
{
    local VERSION="${1:?Release version required.}"
    local DMG_FILE="$(ls build/*.dmg 2> /dev/null | head -n 1)"

    if [ -z "$VERSION" ]; then
        >&2 echo -e "Skipping GitHub release creation (current build does not have a tag)"
        return 0
    fi

    >&2 echo -e "Creating (OSX) GitHub release (tag: $VERSION)"

    create_release_and_upload_asset $VERSION $DMG_FILE
}

function create_beta_release_osx()
{
    local VERSION="${1:?Release version required.}"
    local DMG_FILE="$(ls build/*.dmg 2> /dev/null | head -n 1)"

    >&2 echo -e "Creating (OSX) beta release (tag: $VERSION)"

    mkdir -p ~/.ssh
    ssh-keyscan -p 54433 -H mooltipass-tests.com >> ~/.ssh/known_hosts

    lftp -p 54433 sftp://${SFTP_USER}:${SFTP_PASS}@mooltipass-tests.com \
        -e "set sftp:auto-confirm yes; \
        cd mc_betas; \
        mkdir -p -f $VERSION; \
        cd $VERSION; \
        put $DMG_FILE; \
        cd ..; \
        rm -f updater_osx.json; \
        put build/updater_osx.json; \
        bye"
}

function osx_setup_netrc()
{
    local DIR="${1:?Directory path required.}"

    cat << EOF > $DIR/.netrc
machine api.github.com
    login ${GITHUB_LOGIN}
    password ${GITHUB_TOKEN}

machine uploads.github.com
    login ${GITHUB_LOGIN}
    password ${GITHUB_TOKEN}
EOF

    chmod 600 ${DIR}/.netrc
}

#signing of exe from server
#Usage sign_binary $Path/file.exe
function sign_binary()
{
    set +e
    if [ -f $1 ]
    then
        echo "Signing binary file: $1"

        #sign SHA1
        $HOME/osslsigncode sign \
            -pkcs12 $HOME/cert.p12 \
            -pass "$CODESIGN_WIN_PASS" \
            -h sha1 \
            -n "Moolticute" \
            -i "http://themooltipass.com" \
            -t http://timestamp.comodoca.com \
            -in "$1" -out "${1}_signed1"

        if [ ! $? -eq 0 ] ; then
            set -e
            rm ${1}_signed1
            echo "Failed to codesign SHA1"
            return 255
        fi

        #Append SHA256
        $HOME/osslsigncode sign \
            -pkcs12 $HOME/cert.p12 \
            -pass "$CODESIGN_WIN_PASS" \
            -h sha256 \
            -n "Moolticute" \
            -i "http://themooltipass.com" \
            -ts http://timestamp.comodoca.com \
            -in "${1}_signed1" -out "${1}_signed2" \
            -nest

        if [ ! $? -eq 0 ] ; then
            set -e
            echo "Failed to codesign SHA256"
            return 255
        fi

        mv ${1}_signed2 $1
        rm ${1}_signed1
        fi
    set -e
}


#search for files in folder and sign all exe/dll
#Usage: find_and_sign $Path
function find_and_sign()
{
    Path=$1
    find $Path -iname "*.exe" -type f | while read file; do sign_binary "$file"; done
    find $Path -iname "*.dll" -type f | while read file; do sign_binary "$file"; done
}

function beginsWith()
{
    case $2 in "$1"*) true;; *) false;; esac;
}

function endsWith()
{
    case $2 in *"$1") true;; *) false;; esac;
}

