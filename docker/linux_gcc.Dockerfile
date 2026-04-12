# Filename: linux_gcc.Dockerfile
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

FROM gcc:15 AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        git \
        ninja-build \
        valgrind && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

FROM base AS debug

RUN cmake --preset debug && \
    cmake --build --preset debug --parallel && \
    ctest --preset debug && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/presets/debug/lib/test/LuDataFrameTest

FROM base AS release

RUN cmake --preset release && \
    cmake --build --preset release --parallel && \
    ctest --preset release && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/presets/release/lib/test/LuDataFrameTest && \
    cmake --install build/presets/release --prefix /opt/lugizmo-dataframe && \
    cmake -S docker/package_smoke -B build/package_smoke -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/lugizmo-dataframe && \
    cmake --build build/package_smoke --parallel && \
    ctest --test-dir build/package_smoke --output-on-failure && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/package_smoke/lugizmo_dataframe_package_smoke
