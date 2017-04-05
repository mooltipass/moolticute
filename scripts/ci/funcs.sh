#!/bin/bash

set -e

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
    echo "#ifndef VERSION__H" > $1/src/version.h
    echo "#define VERSION__H" >> $1/src/version.h
    echo "#define APP_VERSION \"$(get_version $1)\"" >> $1/src/version.h
    echo "#endif" >> $1/src/version.h
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

