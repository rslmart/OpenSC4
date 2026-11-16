# enhanced/ — FROZEN (Phase 0 decision)

This tree is **frozen as of Phase 0** (Nov 2026). Do not develop, refactor,
or copy-sync anything here until `authentic/` reaches milestone M4
(Evolution parity — see `../ROADMAP.md`).

## Why

- `enhanced/` was a copy-fork of `authentic/` and had already diverged
  (`developers/`, `network/`, `sim/` exist only here). Two diverging copies
  of the same simulation engine is double maintenance and a parity risk.
- The enhanced track (better graphics, integrated mods, improvements) is
  mostly a renderer/asset/UI layer change. The simulation core must be
  *shared*, not forked.

## Plan when the freeze lifts

Restructure into one engine, two shells:

```
core/        formats, simulation, decompilation registry (single source of truth)
authentic/   platform shell + 1:1 renderer/UI
enhanced/    consumes core, swaps renderer/assets/UI, adds mods
```

Until then: build fixes that keep this tree compiling are acceptable only if
they are *required* by root `CMakeLists.txt` changes; no feature work.
