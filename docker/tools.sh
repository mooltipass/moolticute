#!/usr/bin/env bash

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
function upload_asset_mime() {
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

    ok.sh list_releases "$GITHUB_LOGIN" "$GITHUB_REPO" _filter='.[] | "\(.name)\t\(.id)"' | grep "$NAME" | awk '{ print $2 }'
}

# Get all draft releases
#
# Usage:
#
#     get_draft_releases
#
function get_draft_releases()
{
    local DRAFTS=$(ok.sh list_releases "$GITHUB_LOGIN" "$GITHUB_REPO" _filter='.[] | select(.draft == true and (.assets | length == 0))')
    local ORIGINAL_BRANCH=$(git rev-parse --abbrev-ref HEAD)

    for R in ${DRAFTS[@]}
    do
        RELEASE_TAG=$(echo "$R" | jq -r '.tag_name')

        echo "$ORIGINAL_BRANCH / $RELEASE_TAG"

        git stash

        git pull --tags || true
        git checkout tags/$RELEASE_TAG

        make debian || true

        git checkout $ORIGINAL_BRANCH
        git stash pop
    done
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

    ok.sh release_assets "$GITHUB_LOGIN" "$GITHUB_REPO" "$RELEASE_ID" _filter='.[] | "\(.name)\t\(.id)"' | grep "$NAME" | awk '{ print $2 }'
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
    local delete_url="/repos/${GITHUB_LOGIN}/${GITHUB_REPO}/releases/assets/${ASSET_ID}"

    ok.sh _delete "$delete_url"
}

# Create a release and upload a new assset to that release
#
# Usage:
#
#     create_release_and_upload_asset reponame filepath mimetype
#
# Positional arguments
#
function create_release_and_upload_asset()
{
    local TAG=$1
    local FILE_PATH=$2
    local MIME_TYPE=$3
    local FILE_NAME=$(basename $FILE_PATH)
    local FILE_DIR=$(dirname $FILE_PATH)

    RELEASE_ID=$(get_release_id_by_name "$TAG")

    if [ -z "$RELEASE_ID" ]; then
        echo "Release for tag $TAG doesn't exist. Creating one..."
        RELEASE_ID=$(ok.sh create_release "$GITHUB_LOGIN" "$GITHUB_REPO" "$TAG" name="$TAG" | awk '{print $2}')
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
    pushd $FILE_DIR
    upload_asset_mime "$GITHUB_LOGIN" "$GITHUB_REPO" "$RELEASE_ID" "$FILE_NAME" "$MIME_TYPE" < ${FILE_NAME}
    popd
}