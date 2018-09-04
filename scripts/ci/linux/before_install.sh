#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

if [ "$HOME" != "/home/ubuntu" ] ; then
    #Fix MXE path that was installed in /home/ubuntu
    (cd /home;
     sudo ln -s $(basename $HOME) ubuntu)
fi

# Docker
mkdir -p ${BUILD_DIR}/deb
docker-compose $DOCKER_COMPOSE_CONFIG up --force-recreate -d

# git setup
docker_exec_in $CONTAINER_WIN_NAME "git config --global user.email '${USER_EMAIL}'"
docker_exec_in $CONTAINER_WIN_NAME "git config --global user.name '${USER}'"

#create certificate from env
echo $CODESIGN_WIN_CERT| base64 -d > $HOME/cert.p12
