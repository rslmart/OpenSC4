# Loader Constraints — what the original `.sc4` loader validates

**M2 interop oracle reference.** Reconstructed from the original Mach-O
binary (`Sim City 4 Deluxe Editionsub`) by agent investigation; every row
cites the enforcing address so it stays auditable against the binary.
Used by the clone-and-patch save writer (`cSC4City::SaveToSC4File`).

## Container level — `cGZDBSegmentPackedFile::Open` @ `0x10036a4ac`

| Constraint | Evidence | Severity if violated |
| :--- | :--- | :--- |
| DBPF magic must be valid | `VerifyHeaderRecordIntegrity` @ `0x1003701b5` cmp vs identifier; `FindHeaderRecord` checks `DBPF` @ `0x100370100-120` | archive-open failure (embedded-header fallback exists) |
| Header major version must be 0 or 1 | `0x1003701c7 cmp [rbx+0x174],1; ja fail` | archive-open failure |
| Entry count ≤ `0x7fffffe` | `0x1003701d0` | archive-open failure |
| Index offset/size within file bounds | `0x1003701dc-20d` | archive-open failure |
| Every entry: `offset ≥ 0x60`, `offset+size ≤ fileLength` | `VerifyIndexRecordIntegrity` @ `0x100370291-2a1` | archive-open failure |
| Hole fields structurally consistent (`holeSize/8 == holeCount`; `0/0/0` accepted) | `0x10037020f-248`, `0x10037032f-338` | archive-open failure |

**Freedoms (not validated):** index need not be sorted (`ReadIndexRecord`
@ `0x10036a7c5` inserts into an in-memory tree; first-occurrence-wins for
duplicate TGIs @ `0x10036a869-875`); no alignment/overlap checks; no
container-wide checksum; `dateCreated`/`dateModified` are readable
(`0x10036df82`, `0x10036df8c`) but **never compared**; header minor/user
versions, flags, reserved fields unchecked; no `indexSize == count*20`
equality check (regenerate canonically anyway).

## Compression & DIR

- Compression is decided **by DIR membership**, not per-index-entry flags:
  `DoOpenRecord` looks the full TGI up in the compressed map
  (`0x10036b62f-648`); absent → raw path (`0x10036b74c`), present →
  decompress (`0x10036b66f`).
- **Uncompressed entries require no DIR record.** Raw path uses indexed
  offset/size directly.
- **DIR may be absent entirely** (`0x10036ab64 cmp eax,-1; je success`).
  DIR key: `{type 0xe86b1eef, group 0xe86b1eef, instance 0x286b1f03}`.
- A DIR record listing a raw (uncompressed) entry forces decompression and
  fails the record open — **the regenerated DIR must describe only entries
  we actually write compressed** (i.e. the carried-forward originals).

## COM serializer — `cSC4COMSerializer` (`0x1003cfdd4`)

- The per-object framing CRC `[size][crc][memAddr]` **is verified on load**:
  `0x1003d0df1 call GetCRC; cmp; jne failure` (repeated in `loadSingleObject`
  @ `0x1003d07f3-802`). Coverage = `memAddr` + body, `size-8` bytes capped at
  `250000` (`0x1003d0dd0-ddd`; `GetCRC` → `nRZHash::CRC32` @ `0x1003cff42`).
- COM manifest `{0x499b23fe, 0x299b2d1b, instance 0}` is **mandatory**,
  version must be 1 (`0x1003d0a2f-43`, `0x1003d0aa6`).
- Class subfiles are requested in **ascending numeric class-ID order**
  (manifest normalized through `std::map<uint32,...>` @ `0x1003d0b24-ba7`,
  successor walk @ `0x1003d0f61-fa4`). DBPF index order is irrelevant.
- Missing manifest-listed class subfile → skipped in the class loop
  (`0x1003d0cfb-fd`) but created-vs-expected object counts are compared at
  class end (`0x1003d0f25-2f`) — **not a deletion license**.
- Unknown/unlisted DBPF type IDs are **never enumerated** by COM — carry them
  forward byte-for-byte.

## App-level gates — `cSC4App::LoadCity` (`0x100045686`)

- Application save version must be **62–66 inclusive**
  (`0x10004568e lea ecx,[rax-0x3e]; cmp ecx,4; ja failure`).
- Object identity width is version-dependent: `≤ 0x41` reads 32-bit identity
  (`0x1003d0e06-20`), newer reads 64-bit (`0x1003d0e48-59`). Never change the
  save-version record without changing framing.
- City-link record `{0x49dd6f4c, 0x49dd6f4e, 1}` mandatory, stream version
  must equal 1 (`0x1000457a1-7f7`); `cSC4App::Read` requires app version 2
  (`0x10004459e-a6`).
- PNG thumbnail (`0x8a2482b9`): requirement undetermined
  (`SC4ReadCityRegionViewThumbnails` @ `0x1001d2d56`); carry it forward
  unchanged (clone-and-patch does this for free).

## Writer design consequences (M2 clone-and-patch)

1. Carry every unchanged entry forward **byte-for-byte in original order**
   (compressed form, original DIR records) — no QFS compressor needed.
2. Re-serialize only modified subfiles via the existing byte-exact subfile
   writers (they regenerate `[size][crc][memAddr]` correctly); write them
   **raw** and remove their records from the regenerated DIR.
3. Rebuild index (original order, new offsets/sizes) and header entry
   count/index offset/index size; zero hole fields; clone date fields.
4. Never touch: save-version record, manifest, city-link record, identity
   widths.
