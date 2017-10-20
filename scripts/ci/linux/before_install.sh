#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

sudo add-apt-repository -y ppa:ubuntu-wine/ppa
sudo add-apt-repository ppa:likemartinma/osslsigncode
sudo dpkg --add-architecture i386
sudo apt-get update -qq
sudo apt-get -y install --install-recommends wine1.7 osslsigncode
wget_retry https://calaos.fr/download/misc/InnoSetup5.zip -O $HOME/InnoSetup5.zip

mkdir -p "$HOME/.wine/drive_c/Program Files (x86)/"
pushd "$HOME/.wine/drive_c/Program Files (x86)/"
unzip $HOME/InnoSetup5.zip
popd

pushd $HOME
wget_retry https://calaos.fr/download/misc/mxe_qt591.tar.xz
popd

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

# Docker
mkdir -p ${BUILD_DIR}/deb
docker-compose $DOCKER_COMPOSE_CONFIG up --force-recreate -d

# git setup
$DOCKER_EXEC "git config --global user.email '${USER_EMAIL}'"
$DOCKER_EXEC "git config --global user.name '${USER}'"

#create certificate from env
 echo $CODESIGN_WIN_CERT| base64 -d > $HOME/cert.p12
