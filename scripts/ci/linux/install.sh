#!/bin/bash
set -ev

tar xJf mxe_qt57.tar.xz

unset `env | \
       grep -vi '^EDITOR=\|^HOME=\|^LANG=\|MXE\|^PATH=' | \
       grep -vi 'PKG_CONFIG\|PROXY\|^PS1=\|^TERM=' | \
       cut -d '=' -f1 | tr '\n' ' '`

export PATH=$(pwd)/mxe/usr/bin:$PATH

