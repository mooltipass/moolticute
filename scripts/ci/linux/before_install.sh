#!/bin/bash
set -ev

sudo apt-get install -y wget zip

pushd $HOME
wget https://calaos.fr/download/misc/mxe_qt57.tar.xz
popd

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

