#!/bin/bash
set -ev

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

# Docker
mkdir -p ${BUILD_DIR}/deb
docker-compose $DOCKER_COMPOSE_CONFIG up --force-recreate -d

# git setup
$DOCKER_EXEC "git config --global user.email '${USER_EMAIL}'"
$DOCKER_EXEC "git config --global user.name '${USER}'"

