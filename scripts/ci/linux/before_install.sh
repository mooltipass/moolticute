#!/bin/bash
set -ev

sudo apt-get install -y wget

wget https://calaos.fr/download/misc/mxe_qt57.tar.xz

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

