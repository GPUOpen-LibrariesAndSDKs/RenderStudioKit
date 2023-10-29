FROM ubuntu:20.04

ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && \
    apt-get install -y --no-install-recommends && \
    apt-get install -y python3 \
    python3-distutils \
    build-essential \
    cmake \
    git \
    ca-certificates \
    gpg \
    wget \
    libpython3-dev \
    libx11-dev \
    libgl1-mesa-dev \
    mc \
    zlib1g-dev \
    libssl-dev \
    libglu1-mesa-dev \
    freeglut3-dev \
    mesa-common-dev \
    python3-pip \
    libxt-dev && \
    rm -rf /var/lib/apt/lists/* && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | \
    tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | \
    tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    rm /usr/share/keyrings/kitware-archive-keyring.gpg && \
    apt-get install -y kitware-archive-keyring cmake

ENV DEBIAN_FRONTEND="interactive"

WORKDIR /mnt
CMD mkdir -p Build && \
    echo "[WebRTC]\npath = /dev/null\n[AMF]\npath = /dev/null" > Build/LocalBuildConfig.txt && \
    python3 -u Tools/Build.py -v -ss -sr -sl -sh -sst -sp -si -su -fk && \
    bash -c "cp /usr/lib/x86_64-linux-gnu/libpython* Build/Install/USD/lib" && \
    bash -c "cp /usr/lib/x86_64-linux-gnu/libssl* Build/Install/USD/lib" && \
    bash -c "cp /usr/lib/x86_64-linux-gnu/libcrypto* Build/Install/USD/lib"
