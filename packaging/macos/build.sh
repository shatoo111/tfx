#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
: "${QT_ROOT:?Set QT_ROOT to the Qt 6.8+ macos directory}"
cmake -S . -B build/macos -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_ROOT" -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
cmake --install build/macos --prefix "$PWD/dist/macos"
bash packaging/macos/package.sh "$PWD/dist/macos/TFX.app" "$PWD/dist/macos/TFX-macos.zip"
printf '%s\n' "Unsigned local app: $PWD/dist/macos/TFX.app"
