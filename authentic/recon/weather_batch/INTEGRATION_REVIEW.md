# Independent batch review — 2026-11-15

Independent reviewer record; private session locator omitted.

**Bounded component acceptance: no blocking semantic or integration findings. G3.5 remains in progress.**

Reviewed HEAD `02472c840e058386dff665a62f1b342e937b053c` plus the uncommitted overlay. All eleven source hashes match the [integration handoff](/Users/russellmartin/Personal/OpenSC4/authentic/recon/weather_batch/INTEGRATION_HANDOFF.md); binary and record-header hashes match the supplied identities. I derived the operation inventory from four fresh `--no-verify --disasm` calls before inspecting implementation, then independently dumped the instruction bytes.

| Method | Review result |
|---|---|
| PositionToCell `0x1000e487a` | Correct binary32 DIV then CVTT truncation. Checks distinguish reciprocal multiplication, rounding before truncation, limits, subnormals and second-axis rejection preserving both outputs. |
| CellToPosition `0x1000e4896` | Correct int32→binary32 rounding **before** multiplication. Checks discriminate widened multiplication, ties, extrema, subnormals and overflow rejection. |
| CellToStandard `0x1000e48ba` | Bytes confirm **X supplies both products**. Unsigned multiplication preserves low32 wrapping. Corrected `INT_MIN * -5 → INT_MIN` expectation is valid. |
| StandardToCell `0x1000e48d0` | Correct independent signed divisions. Both zero divisors and overflow pairs are rejected before division or output; checks cover second-axis failure and mixed invalid pairs. |

The mappings match the actual record declaration: float widths `+0x58/+0x5c`, signed scales `+0x70/+0x74`. Status returns, restricted domains and atomic rejection are explicitly adapter policy. No cheap zero shortcuts were found.

Integration is correctly wired: four core and four test TUs, strict FP flags on both float implementations/tests, standalone mains excluded, corrected namespaced runner, and one suite `Check` per runner. The decoded-record check exercises the adapters; it is not original-runtime proof.

The retained logs support the initial namespace link failure, subsequent **870/1**, then **871/0**, including 417 cities and 2,185,774 applicable byte-exact records. The final rebuild recompiles the multiplication test only, consistent with the arithmetic remaining unchanged. The earlier worker standalone-pass claim remains unreproduced historical evidence. Registry has no diff; STATUS reflects 871 passes.

I reran the existing translated-x86 executable successfully: **114 checks; 159 checks; 12 checks; 24 cases**, separately counted. No builds or global verifier were run.

One non-blocking coverage limitation: multiplication’s record-preservation assertion checks selected fields rather than the complete record. Source inspection shows no record writes.

Acceptance is limited to the documented masked-RNE, gradual-underflow, nonaliasing adapter domain. Blind reimplementation, live original comparison/Phase B, gameplay scheduling and dirty-save proof remain pending. Normal integration is unsanitized; native x86 hardware, WASM, GNU and MSVC remain untested. A separately reviewed typed paired fixture following W6 is a possible next proof boundary; no such harness or live window is approved by this review.
