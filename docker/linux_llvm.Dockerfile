# Filename: linux_llvm.Dockerfile
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

FROM debian:trixie-slim AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        gnupg \
        lsb-release \
        ninja-build \
        valgrind \
        wget && \
    rm -rf /var/lib/apt/lists/*

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 19 all && \
    rm -f llvm.sh

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        clang-19 \
        libc++-19-dev \
        libc++abi-19-dev \
        lld-19 && \
    rm -rf /var/lib/apt/lists/*

ENV CC="clang-19"
ENV CXX="clang++-19"
ENV LD="ld.lld-19"
ENV CXXFLAGS="-stdlib=libc++"
ENV LDFLAGS="-stdlib=libc++"

WORKDIR /src

COPY . .

FROM base AS debug

RUN cmake --preset debug && \
    cmake --build --preset debug --parallel && \
    ctest --preset debug

FROM base AS release

RUN cmake --preset release && \
    cmake --build --preset release --parallel && \
    ctest --preset release && \
    cmake --install build/presets/release --prefix /opt/lugizmo-dataframe && \
    cmake -S docker/package_smoke -B build/package_smoke -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/lugizmo-dataframe && \
    cmake --build build/package_smoke --parallel && \
    ctest --test-dir build/package_smoke --output-on-failure
