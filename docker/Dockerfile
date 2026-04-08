FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG PICO_SDK_TAG=2.2.0

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    cmake \
    ccache \
    gcc-arm-none-eabi \
    git \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    ninja-build \
    python3 \
    python3-pip \
    rsync \
    unzip \
    xz-utils \
    zip \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --branch ${PICO_SDK_TAG} --depth 1 https://github.com/raspberrypi/pico-sdk.git /opt/pico-sdk \
    && git -C /opt/pico-sdk submodule update --init --recursive

ENV PICO_SDK_PATH=/opt/pico-sdk
ENV CCACHE_DIR=/tmp/.ccache
WORKDIR /workspace

CMD ["bash"]
