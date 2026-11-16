#!/bin/bash
# Build the OpenSC4 game-oracle dylib. It MUST be x86_64 to match the
# shipped game (non-fat x86_64 binary; runs under Rosetta 2 on Apple
# Silicon, and the injected dylib must match the process architecture).
set -euo pipefail
cd "$(dirname "$0")"
OUT=opensc4_oracle.dylib
SDK=$(ls -d /Library/Developer/CommandLineTools/SDKs/MacOSX15*.sdk 2>/dev/null | sort | tail -1)
if [ -z "${SDK:-}" ]; then
  SDK=$(xcrun --show-sdk-path)
fi
clang++ -arch x86_64 -std=c++17 -O2 -Wall -Wextra -ffp-contract=off \
    -dynamiclib \
    -isysroot "$SDK" \
    -install_name "$PWD/$OUT" \
    -o "$OUT" oracle_dylib.cpp
echo "built $PWD/$OUT (sdk: $SDK)"
file "$OUT"
