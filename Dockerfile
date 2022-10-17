ARG BASE_IMAGE=ubuntu:20.04

FROM ${BASE_IMAGE} AS builder

WORKDIR /build

ARG BUILD_PACKAGES='\
  git \
  g++ \
  make \
  libdw-dev \
  libunwind-dev \
  libiberty-dev \
'

RUN apt-get update -qq && \
  DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $BUILD_PACKAGES

COPY ./include ./include
COPY ./src ./src
COPY ./tests ./tests
COPY Makefile Makefile

RUN make all
