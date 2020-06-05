FROM debian:stretch
LABEL maintainer="Sierra Developers <dev@sierracoin.org>"
LABEL description="Dockerised SierraCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
