#!/bin/bash
# Launch the shipped SimCity 4 Deluxe (macOS) with the OpenSC4 oracle
# dylib injected. Read-only Phase 2 oracle; see oracle_dylib.cpp.
#
# Channels appear under ~/.opensc4-oracle/ : oracle.log (output) and
# command (input; one line per command, e.g. "ping", "redump",
# "read f32 0x100b175d4").
#
# No app-bundle modification: injection is a launch-time env var only.
set -euo pipefail
GAME="/Users/russellmartin/Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe/Sim City 4 Deluxe Edition.app/Contents/MacOS/Sim City 4 Deluxe Editionsub"
# Refuse to double-launch: stale instances race on the shared command file and
# answer with old dylib code (bit us twice in testing). Kill them first.
if pgrep -f "Sim City 4 Deluxe Editionsub" >/dev/null 2>&1; then
  echo "killing stale Sim City 4 instance(s) before launch:" >&2
  pkill -9 -f "Sim City 4 Deluxe Editionsub" || true
  for _ in 1 2 3 4 5 6 7 8 9 10; do
    pgrep -f "Sim City 4 Deluxe Editionsub" >/dev/null 2>&1 || break
    sleep 0.5
  done
fi
DYLIB="$(cd "$(dirname "$0")" && pwd)/opensc4_oracle.dylib"
if [ ! -f "$DYLIB" ]; then
  echo "oracle dylib missing: $DYLIB (run ./build.sh first)" >&2
  exit 1
fi
mkdir -p "$HOME/.opensc4-oracle"
: > "$HOME/.opensc4-oracle/oracle.log"
# Steam check (Mac_Steam_Authentication @ 0x100002b44: SteamAPI_Init ->
# "NeedsSteam"; SteamApps()->vt0 -> "InvalidSteamAccount"). steam_appid.txt
# in the process CWD is Valve's documented direct-launch mechanism; the Steam
# client must be running. Keep the file OUT of the app bundle.
echo 24780 > "$(dirname "$0")/steam_appid.txt"
cd "$(dirname "$0")"
# Intro-movie skip: the oracle constructor sets the binary's own
# CQTPlayer::mbsNCTSkipMovie flag @ 0x100b713b1 (honored by CQTPlayer::Open /
# Play / IsPlaying), so NCT_NewIntroMovieSequence finishes instantly and
# SC4AppStart runs immediately. Set OPENSC4_ORACLE_ALLOW_MOVIES=1 to watch the
# movies instead (also disables the legacy open/fopen interposition fallback).
export OPENSC4_ORACLE_ALLOW_MOVIES="${OPENSC4_ORACLE_ALLOW_MOVIES:-0}"
DYLD_INSERT_LIBRARIES="$DYLIB" exec "$GAME" -intro:off
