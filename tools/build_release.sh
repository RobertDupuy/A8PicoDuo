#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to Pico SDK 1.5.1}"
python3 tools/build_cas_runtime.py
cmake -S firmware/engine -B build/engine -DCMAKE_BUILD_TYPE=Release
cmake --build build/engine -j4
mkdir -p releases
cp build/engine/a8_pico_cart.uf2 releases/A8Duo-CAS-v1-U1-Engine.uf2
python3 tools/make_cas_fixtures.py
cp tests/cas-fixtures/BOOTTEST.CAS releases/CASTEST.CAS
