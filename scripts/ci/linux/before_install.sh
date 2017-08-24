#!/bin/bash
set -ev

sudo add-apt-repository -y ppa:ubuntu-wine/ppa
sudo dpkg --add-architecture i386
sudo apt-get update -qq
sudo apt-get -y install --install-recommends wine1.7
wget https://calaos.fr/download/misc/InnoSetup5.zip -O $HOME/InnoSetup5.zip

mkdir -p "$HOME/.wine/drive_c/Program Files (x86)/"
pushd "$HOME/.wine/drive_c/Program Files (x86)/"
unzip $HOME/InnoSetup5.zip
popd

pushd $HOME
wget https://calaos.fr/download/misc/mxe_qt57.tar.xz
popd

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

