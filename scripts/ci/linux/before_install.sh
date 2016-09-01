#!/bin/bash
set -ev

sudo apt-get install -y wget zip curl

pushd $HOME
echo "Current checked out branch: $(git symbolic-ref -q HEAD)"
wget https://calaos.fr/download/misc/mxe_qt57.tar.xz
popd

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

