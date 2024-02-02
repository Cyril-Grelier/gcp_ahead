FROM ubuntu:22.04

ENV HOME /root

SHELL ["/bin/bash", "-c"]

RUN apt update && apt -y --no-install-recommends install \
    build-essential \
    clang \
    clang-format-12 \
    gcc-12 \
    cmake \
    gdb \
    wget \
    git \
    openssh-server \
    gdb \
    pkg-config \
    valgrind \
    systemd-coredump \
    libomp-dev \
    ca-certificates \
    python3-venv \
    zip \
    unzip \
    curl \
    neovim
