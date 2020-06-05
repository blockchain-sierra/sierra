#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-sierracoin/sierrad-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/sierrad docker/bin/
cp $BUILD_DIR/src/sierra-cli docker/bin/
cp $BUILD_DIR/src/sierra-tx docker/bin/
strip docker/bin/sierrad
strip docker/bin/sierra-cli
strip docker/bin/sierra-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
