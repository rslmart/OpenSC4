# G3.5 coordinate batch integration handoff — 2026-11-15

Source HEAD02472c840e058386dff665a62f1b342e937b053c plus the uncommitted
sources/tests/CMake/verifier/header below. No game operation, production acceptance
or G3.5 completion. This handoff describes the tested overlay, not HEAD alone.
The three completed initial workers were Astra/Astra/Luna; the remaining
signed-division worker used Sol under user-authorized bounded-leaf-v2. Existing
worker reports remain historical and are not relabeled as final integration proof.

## Changes and actual failures

Conductor registered all four leaf implementations and test TUs in normal CMake.
Clang strict FP applies to the two float implementations and their tests; GNU
has explicit no-fast-math/rounding/contraction flags, not tested here. Other
compiler/hardware/WASM proof remains unexercised. The public free-function API
uses the actual WeatherSimulatorSaveRecord; no native layout or serialization
field order was changed. Caller guards remain distinct from native branches.

Each internal test runner contributes ONE suite Check result. Internal assertions
are separate units and are never added to the suite total. Added one check that
uses the actual decoded Big City Tutorial weather record: cell(-2,3)→world→cell
and forward-standard(2,5)→reverse demonstrate both normal coordinate conversion
and native X reuse. This establishes record execution, not natural tick wiring
or original-game hydration.

First full build failed because the multiplication test runner was namespaced,
but the initial integration declared it global. Fixed the declaration and call;
second build succeeded. First full verifier then reported870pass/1fail: the
multiplication worker test expected zero for INT32_MIN*-5, whereas low32 IMUL
produces INT32_MIN. Corrected that fixed expected value, removed a vacuous
arithmetic-only assertion, added per-check diagnostics, and marked unused inputZ
in the implementation. No arithmetic implementation change. Third build passed.
The worker's earlier standalone-pass claim did not reproduce in this integrated
run; retain both claims/evidence rather than erase that discrepancy. No assertion
that model tier caused the error or that one ticket measures comparative quality.

## Exercised gates

- Normal `cmake --build build`: final exit0, after the recorded link correction.
- Integrated verifier:871pass/0fail; all417cities and2,185,774applicable byte-exact
  records. Raw build/verify failure and success logs are adjacent integration_*.log.
- Integrated per-method output: PositionToCell38cases/114checks/0fail;
  CellToPosition159checks/0fail; CellToStandard12checks/0fail;
  StandardToCell24cases/0fail. Counts use different units; no combined score.
- x86_64 leaf batch compiled with explicitSDK26.5, -O2 -ffp-model=strict
  -ffp-contract=off; all four runners returned0 under host translation. This is
  translated x86, not native hardware x86. Exact output in integration_x86-results.log.
- Existing static guardrail all8source/test files passed with0diagnostics.
- Registry sync:11798addresses, no source diff. STATUS regenerated from captured
  final verifier log, with current suite count. No assumption that citations,
  Tier3/AUTO coverage or number of methods equals finished game progress.
- Individual worker sanitizer checks are retained in their handoffs. The final
  normal integrated core is not sanitizer-instrumented; no new combined sanitizer
  run is claimed. Leak detection was unavailable in the Sol leaf run.

## Remaining gates and ownership

One independent Astra semantic/integration batch review using fresh original
bytes, then scoped fixes/discriminators. Blind reimplementation remains pending
unless separately evidenced; don't treat normal reviewer source inspection as
blind. No live original comparison, PhaseB, dirty-save or monthly scheduling
claim. The functions are linked into the production core and used by the verifier;
no gameplay call-site is added by this slice.

Potential next original proof is a reviewed typed paired fixture (existing W6
pattern) because these four original bodies only consume the two width/scale
fields; no confirmed safe harness or live approval yet. Reuse record data and
production implementation. One conductor owns original process/commands, with
fresh original/source/installed identities and protected preferences/watchdog/
stop policy before launch. G3.4's oracle ownership must be coordinated, and G3.3
remains reserved. Native fault/invalid conversion cases remain outside the
bounded adapter and must not be executed in-process as tests. Compiler matching
is deferred; no new tool or compiler is required for this route.

## Tested source hashes

```
950cbd9c30eb271c72e2de2dd935c0e952d57ef283b8fc05b87874ad2415dc6c  authentic/include/opensc4/weather_coordinates.h
636bfeb5cfa19012bf1c1a1ed9d1da8a3205d31b98cace0649802003bab42ffe  authentic/CMakeLists.txt
56495964e9bab175fb05074e0ee2491107d8cc8bcae1c8efcc2e1b4cb341aa7a  authentic/tests/verify_sot.cpp
14d2f5b1379b16eb482b7151d41657e9a0844d6f0ef4e0d75a79c32545544cfa  authentic/src/weather_position_to_cell.cpp
004644ca21b80638398c341279d160ba70f1fd111971cbb707f1c5dff8852c82  authentic/tests/weather_position_to_cell_checks.cpp
203d613a7bee7dc1caa19b47b8f05ac39c09e894070f74d848760459cd6443c7  authentic/src/weather_cell_to_position.cpp
aafe10ee6914776b47848892776cad39a675e90a853ed913d0c1a5ba04dda7ec  authentic/tests/weather_cell_to_position_checks.cpp
52eb306625ca724427ca40d82a192bd3a3de4811677a1e55d9096d248a926014  authentic/src/weather_cell_to_standard.cpp
bb63cadac1ae30be23090105469c49005119be3eca4472df751450c217b166d4  authentic/tests/weather_cell_to_standard_checks.cpp
2ea5c9eaf3f9bac09763e2d28c82f2a1840127c666396e3dda07da03390ff3bd  authentic/src/weather_standard_to_cell.cpp
e4f106591256b23c0eb2b33aa3dd6880068134044149a7de803dd0aa24048489  authentic/tests/weather_standard_to_cell_checks.cpp
```

## Review disposition — 2026-11-15

INTEGRATION_REVIEW.md preserves independent Astra acceptance of all four
methods and integration, fresh original disassembly/byte checks, eleven source
hash matches and translated-x86 rerun. No blocking finding. Multiplication's
record-preservation test samples selected fields; reviewer inspected no record
writes. Blind reimplementation/original comparison/gameplay scheduling remain
pending. Preserve this as a component milestone, not G3.5 completion.

The translated-x86 driver is retained as integration_x86_driver.cpp. Reproduce
from repository root with explicit SDK26.5 and current sources:

```sh
xcrun clang++ -std=c++20 -arch x86_64 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk -O2 -ffp-model=strict -ffp-contract=off -Iauthentic/include authentic/src/weather_position_to_cell.cpp authentic/src/weather_cell_to_position.cpp authentic/src/weather_cell_to_standard.cpp authentic/src/weather_standard_to_cell.cpp authentic/tests/weather_position_to_cell_checks.cpp authentic/tests/weather_cell_to_position_checks.cpp authentic/tests/weather_cell_to_standard_checks.cpp authentic/tests/weather_standard_to_cell_checks.cpp authentic/recon/weather_batch/integration_x86_driver.cpp -o /tmp/opensc4-weather-batch-x86
/tmp/opensc4-weather-batch-x86
```
