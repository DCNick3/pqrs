#!/bin/bash

set -e

cd "$(dirname "$(realpath "$0")")"

rm -rf cmake-build-release-emscripten/
mkdir -p cmake-build-release-emscripten/
cd cmake-build-release-emscripten/

emcmake cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ..
cmake --build .

cd ..
rm -rf emscripten/js/bin/
mkdir -p emscripten/js/bin/
cp -r cmake-build-release-emscripten/emscripten/cpp/pqrs-emscripten* emscripten/js/bin/
