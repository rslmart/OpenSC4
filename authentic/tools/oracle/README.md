# Game oracle (in-process, injected)

Runs inside the live SimCity 4 Deluxe (macOS) process via `DYLD_INSERT_LIBRARIES`,
giving the decompilation loop a runtime oracle: read live game memory at any of
our static (disassembly-time) addresses. Established by
`authentic/recon/dossier_game_plugin_abi.md`: the Mac port's built-in plugin
loader is a stub, but injection is viable (codesign flags 0x0, x86_64 PIE).

Clean-room tooling — not part of the authentic recreation.

## Usage

```bash
cd authentic/tools/oracle
./build.sh                      # build opensc4_oracle.dylib (x86_64, matches the game)
./run_oracle_game.sh            # launch the game with the oracle injected
tail -f ~/.opensc4-oracle/oracle.log
echo "read f32 0x100b175d4" > ~/.opensc4-oracle/command
```

Commands (multi-line batches are processed in order; the file is consumed):
- `ping` — liveness check (`RPC pong` in the log)
- `redump` — re-run the tunables battery
- `read f32|f64|u32|u64 <static-addr-hex>` — read one value (bit patterns included)
- `read mem <static-addr-hex> <n-bytes>` — hex dump (n ≤ 256)
- `find <needle-hex> [max-hits]` — scan all writable regions for the value
  (32-bit needles match at 4-byte-aligned offsets; 64-bit needles need two
  consecutive halves). Locates live objects by vptr, e.g.
  `find 0x100a67010` = &__ZTV7cSC4App+0x10 finds the cSC4App instance.
- `call <static-addr-hex> [args-hex ...]` — invoke a game function on the
  MAIN thread via the dispatch main queue (x86_64 SysV integer ABI, up to 6
  u64 args, u64 return). Member calls need the instance as arg 1 (get it
  via find). Call only functions you know are live — calling an unconstructed
  subsystem (e.g. the GZCOM() magic static at menu time) crashes the game.

## Isolated pollution shared-kernel proof (W6)

`prove-pollution-grid` queues all six original pollution member methods on
`dispatch_get_main_queue()` and compares each against the exact shared
`authentic/include/opensc4/pollution_grid_kernel.h`. It requires the matching
original executable loaded with the rebuilt oracle, but no city pointer or
loaded city; menu state is sufficient. The parent/conductor owns launching,
restarting and invoking it:

```sh
printf '%s\n' 'prove-pollution-grid' > ~/.opensc4-oracle/command
```

Each case starts with two separately initialized, aligned POD fixtures, one
for original machine code and one for the shared kernel. Only fixture memory
is changed. Synthetic vtables implement grid get/shift/set, mode and own-query
callbacks. Kernel callbacks resolve the fixture's current object slots and
vtables on every dispatch. No real city grid or global vtable is patched.

The defined matrix contains **8,016 paired cases** (not assertions or natural
game call counts):

| Original method | Static address | Cases |
| --- | --- | ---: |
| SetIsRadioactive | `0x10009a738` | 84 |
| SetAirValue | `0x10009a790` | 420 |
| SetWaterValue | `0x10009a7e8` | 420 |
| SetGarbageValue | `0x10009a840` | 2,520 |
| IsActiveLandfill | `0x10009a924` | 36 |
| SetIsActiveLandfill | `0x10009a940` | 4,536 |

Coverage includes unsigned high coordinates, shifts 0/1/31/32/33/-1/256,
16-bit edges and wider signed inputs, garbage old-lowbit clearing and active
bit preservation, full-width mode comparisons, unconditional mode calls,
query AL values 0/1/2 with upper bits, counter floor/wrap and untouched dirty
bytes on no-op paths. Boolean arguments are canonical false/true.

Every `PG INPUT` line identifies the inputs and case result. Both sides then
log final dirty bytes, counter, all four ushort storage cells and return
(`void:0` is a placeholder, **not** a captured setter return). Query return is
captured as raw uint32. Ordered `event` lines use kinds 0=get, 1=shift, 2=set,
3=mode, 4=own-query; `raw` is the callback return for getters and the complete
ECX bit pattern for setters. Each event also records coordinates, grid identity
and pre-callback counter/dirty state. Full raw ECX and stored low16 are compared
separately. Any trace overflow fails the case. The command compares all of
these observations, not only return values.

Require six `PG METHOD` lines with the counts above, zero mismatches, and
`PG TOTAL cases=8016 mismatches=0 END`; an interrupted or incomplete log is
not a pass. The command emits every case/trace, so expect a sizeable log and
main-thread work. Capture the full log and binary/source identities for evidence.

Limits: the grid callback is an isolated one-cell ushort storage sink with
coordinate traces, not the original grid allocator/indexing or bounds behavior.
Getters are configured raw return values; upper-bit fixtures intentionally
stress the ABI beyond ordinary ushort storage. Callback-driven slot/vtable
replacement and callback side-effect mutation are not covered by this matrix.
This original-vs-shared proof does not establish real-city tick integration,
natural game invocation counts, or Phase B substitution. No substitution is
performed. The implementation/build alone is not a live parity result.

## Self-driving commands (the game drives itself)

All execute on the main thread at a safe pump point. The app instance is
resolved O(1) via `cGZFramework::mpApp` @ 0x100b21238 (= cSC4App + 0x48,
set by SC4AppStart @ 0x100003983; cross-checked against the vptr-validated
heap scan). `SL::spSC4` @ 0x100b21258 = cSC4App + 0x18.

- `state` — appState (1 = menu/region, 2 = city) + city pointer (app+0xa0)
- `regions` — list all regions via GetNation() (spSC4 vt+0x110 →
  cISC4Nation: count vt+0x30, region-at vt+0x58, name obj vt+0x18 → vt+0x40)
- `region <name>` — cSC4WinRegionScreen::LoadRegion @ 0x10023b130
  (case-insensitive exact match). If a city is open, exits to region view
  first via RequestGoToRegionView(false) @ 0x1000451e6.
- `cities` — enumerate the region screen's city tiles (screen+0x1d0..0x1d8,
  stride 0xf0): index, (x,z) coords, size, established, name, .sc4 path
  (via region vt+0x58 GetRegionalCity(x,z) → city vt+0x108 name /
  vt+0xf8 path / vt+0x158 established).
- `launch-city <name|#idx|x,z>` — load any city: GetOrCreateRegionalCity
  @ 0x10023ae9c (create=true) → LaunchRegionalCity @ 0x10023b63c →
  LoadCity with the real regionalCity ref (the crash-free path; direct
  LoadCity from the menu crashed because app+0x98 was null). Match is exact
  first, then unique substring (name or path). Prints city, terrain
  (city+0x390), and sim (city+0x258) pointers.
- `region-view` — exit city view to the region screen.
- `sim resume` — drain user pause (sim+0x48) and hidden pause (sim+0x4c)
  via Resume @ 0x1001f11a0 / HiddenResume @ 0x1001f11c4, default to speed 1
  if unset; `sim pause` — Pause @ 0x1001f1100; `sim speed <0..4>` —
  SetSimSpeed @ 0x1001f093e (gated by sim+0x5c enabled); `sim state` —
  read JDN (GetSimDateNumber @ 0x1001f19d8) + all pause counters.
- `saveas <path>` — cSC4App::SaveCity(cRZString const&, bool) @ 0x100046036
  (the FULL save: cSC4App::Write @ 0x100044510 serializes app+city+
  regionalCity; 151 records / 141 types on a test city vs 5 types for the
  obliteration variant). NOTE (decoded + observed): the file lands at the
  city's DEFAULT path (City - <Name>.sc4), and the game then rewrites every
  NEIGHBOR city file (SL::spNeighborsSim @ 0x100b21448 → vt+0xa0 →
  SC4WriteRegionalCity @ 0x1001e9799 per neighbor). Copy the file yourself
  for a non-destructive checkpoint. Do NOT use SaveCityForObliteration
  @ 0x1000475cc for checkpoints — it writes terrain+flora only (verified:
  0x8a91e7e3/0x8a91e7e0/0xa9dd6ff4 + ~3000 flora records, no development).
- `load`/`saveas` legacy paths remain for forensics.

Background-pause suppression: launch/region commands call
EnableFullGamePauseOnAppFocusLoss(false) @ 0x10000f2ba (app+0x6b0 and the
prefs mirror prefs+0x4a4) so the sim keeps running while the window is
unfocused. Verified live: JDN advanced ~1 day/sec at speed 1 for minutes,
with app+0x6b0/0x6b1 reading 0. Tutorial cities (Konradshöhe) self-pause at
scripted date milestones — expected, city-specific.

Single-instance guard: the launcher kills stale game processes, and the
dylib claims command-file ownership via ~/.opensc4-oracle/owner (newest pid
wins). A stale instance once answered commands with an old dylib and stale
heap pointers — the guard prevents that class of corruption.

## Intro-movie skip

The constructor sets the binary's own CQTPlayer::mbsNCTSkipMovie flag @
0x100b713b1 (honored by CQTPlayer::Open @ 0x100724247, Play @ 0x100724468,
IsPlaying @ 0x10072461e → returns 0), so NCT_NewIntroMovieSequence
@ 0x100003147 completes instantly and SC4AppStart populates SL::spSC4
immediately — the game is at the region screen ~25 s after launch instead
of ~70 s. OPENSC4_ORACLE_ALLOW_MOVIES=1 watches the movies instead.

Known live addresses (menu state): cSC4App instance ≈ heap (find by vptr
0x100a67010); GetCity @ 0x10001231a; cSC4App::GetAppState @ 0x100013634.

The battery runs at constructor time (shipped-default values) and again ~30 s
after launch (post-exemplar-load values) — the diff between the two is exactly
what the terrain tuning exemplar set at runtime.

## Phase 3 (in progress)

- `call` command: invoke game functions at static addresses (x86_64 SysV
  argument marshalling) — turns every decompilation dispute into an
  experiment.
- **vtable swap (strangler fig)**: `vtread <vtable-static-hex> <slot-off-hex>`,
  `swap <vtable-static-hex> <slot-off-hex>`, `unswap ...`, `swapstats`.
  Patches a vtable slot (mprotect dance on the game's __DATA_CONST) to a
  counting thunk that forwards to the original target (6-arg integer ABI).
  Proven end-to-end on cSC4Simulator::GetSimDateNumber (vt+0x40 of vtable
  0x100a841d0): patch -> forward (exact JDN through the thunk) -> count ->
  clean restore, game unharmed. Phase A (mechanics + call counting) is DONE;
  Phase B points slots at RECONSTRUCTED implementations so the live game
  exercises our code — the strongest L4 gate. The Mac port's built-in
  plugin loader is a stub, so oracle vtable substitution IS the strangler
  fig on this binary.
- **Phase B wave 2 (substituted cSTETerrain implementations)**:
  `prove-cost|prove-cell|prove-altitude <terrain-hex>` — equivalence proofs
  vs the originals on live data (bit-exact); `phaseb-swap
  <brushradius|cost|water|land|altitude>` — patch the slot to OUR counting
  shim (cSTETerrain vtable 0x100aea6b0; BrushRadius vt+0x1e8,
  GetCostPerBrushApplication vt+0x1f0, CellHasWater vt+0x110,
  CellHasLand vt+0x118, GetAltitude vt+0x060); `phaseb-stats` — call
  counters; `unswap 0x100aea6b0 <off>` restores. Live-proven on Spandau:
  0 mismatches across all proves (GetAltitude: 7821 bit-exact float
  points across both triangle diagonals and four finite out-of-bounds inputs), then swapped for
  ~33 s: the game executed OUR GetAltitude 1.98M times and OUR
  CellHasWater 85k times, counters frozen after restore, game unharmed.
  Recon bonus: CellHasLand/BrushRadius/GetCost are only dispatched during
  god-mode terraforming (0 calls in normal play); CellHasLand's only
  consumers are elsewhere (tract developer paths). GetAltitude's float-ABI
  substitution works (LocationIsInBounds gate self-virtual via vt+0x58,
  magic-constant truncate 0.0625/-0.5/12582912, 36-byte vertex records).
  The oracle dylib is built with -ffp-contract=off so our float op order
  matches the binary's unfused mulss/addss under Rosetta.
  GetAltitude now shares `include/opensc4/terrain_altitude_kernel.h` with
  the standalone core; the oracle adapter retains the original virtual bounds
  dispatch and adapts only the live vertex-record layout. The historical
  7,821-point result above predates that shared kernel and did not test NaN
  coordinates or signed-zero ties. The expanded `prove-altitude` battery
  adds 16 explicit bound/nonfinite inputs and two synthetic terrain fixtures
  with asymmetric slopes and signed-zero coordinates (7,839 comparisons).
  Synthetic fixtures call the original function and the shared kernel without
  modifying real terrain. On 2026-11-13 the rebuilt oracle passed all 7,839
  comparisons with zero mismatches on a disposable Spandau copy; the original
  and shared kernel returned matching bits for both signed-zero fixtures.
  Phase B then executed the shared GetAltitude 2,133,464 times over 24 seconds;
  restoring the original slot froze the counter. Revision/content-bound
  evidence is in `authentic/recon/terrain_getaltitude/EVIDENCE.json`. New
  builds require a new live run; this result covers this method and fixture.
- audit-tool client (`--ask-game`) so reviewers get "ask the game" as a
  first-class dispute-settling move.

## Safety

Strictly read-only in Phase 2 (plain global reads only; never calls game code).
A crashing oracle crashes the game — acceptable for a test rig; never load a
city you care about while experimenting.

## Gotchas learned in Phase 2 (read before touching the dylib)

- **No `std::string` (or any libc++ object) for state that must survive across
  game init.** The game ships an ancient bundled C++ runtime; our SDK-era
  libc++ coexists with it badly. A path held in a `std::string` global read
  back correctly through `%s` but made `open()` fail with ENOENT until the
  string was replaced with a plain `char` array (byte-identical content).
  Rule: all persistent state = POD / char buffers; libc++ types only as
  function-local scratch if at all.
- **No stdio (`fopen`/`fprintf`) for the RPC loop.** Symptomatic failures
  mid-init; use raw `open`/`read`/`write` with a held log fd and a
  self-healing reopen on EBADF/EIO.
- The game loads at its static base (slide = 0 observed) but the oracle still
  computes the slide at startup — do not assume.
- `lipo`: build `-arch x86_64` only; link with an older SDK
  (MacOSX15 works) — the CLT's current SDK TBDs fail tapi parsing for x86_64.
- The launch script passes `-intro:off` and the game's `intro-movie`
  preference is set false, but those only gate the GAME CORE's intro. The Mac
  port also plays native QuickTime movies (GameData/Video/{EA_logo,Aspyr,
  SC4Intro}.mov) unconditionally via NCT_NewIntroMovieSequence @ 0x100003147
  called from _main @ 0x100002c01 BEFORE the core cmdline exists. The oracle
  dylib interposes open/fopen to fail those .mov paths (DYLD interposition),
  which skips the native intro with zero bundle modification.

## Isolated current-capacity proof

`prove-pollution-capacity` dispatches `GetCurrentCapacity @0x1000992cc`
against the shared `pollution_capacity_kernel.h` on synthetic POD objects.
The conductor must load this rebuilt oracle, then invoke:

```sh
printf '%s\n' 'prove-pollution-capacity' > ~/.opensc4-oracle/command
```

No city pointer is required. The command does not patch game state or slots.
It temporarily selects default SSE rounding/masks (MXCSR `0x1f80`) and restores
all caller MXCSR bits afterward. It allocates only temporary original curves,
disposing their storage through the original `operator delete(void*)` stub.

Require `CAP-TOTAL method=1000992cc` with zero mismatches and the following
`CURVE-TOTAL`; a partial log is not a pass. There are 1,702 candidate capacity
fixtures, expected 1,437 comparisons and 265 explicit null-backed exclusions
(the observed counts are authoritative). Each logs return low byte, output
bits, fixture flags, raw property scalars, and ordered property/power traces.
`inf_response_zero_capacity` must be positive to establish that the generated
infinity-times-zero NaN path was reached. Callback traces cover interface
calls, not internally unhooked curve constructor/evaluator/destructor calls.

Both comparison arms deliberately use the **original curve implementation**.
This proves only the capacity kernel, not the production curve helper.
`CURVE-SET`, `CURVE-RECORD`, `CURVE-EVAL`, and retained `CURVE-EMPTY` records
provide separate original-only goldens for native production replay; these
are explicitly unscored until that replay. Fresh/all-skipped null-backed
curves are never evaluated. See
`authentic/recon/pollution_current_capacity/ORACLE_HANDOFF.md` for coverage,
trace definitions, exclusions and acceptance instructions. Build success is
not live proof; this command has not been executed by its implementation worker.
