#!/bin/bash

set -e

#Usage: get_version /path/to/repo
function get_version()
{
    repo=$1
    pushd $repo > /dev/null
    git describe --long --tags --always
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

