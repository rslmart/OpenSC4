# STATUS.md — Authentic decompilation progress

<!-- AUTO-GENERATED: regenerate with `python3 tools/sc4_decomp_audit.py --status-md STATUS.md` -->

Generated: 2026-11-15
Verify suite: **871 [PASS]** checks (127 symbols VERIFIED)
Symbols scanned: 43333 `__TEXT,__text`; 12746 cited/covered.

## Overall completion (whole authentic project)

- **5.7% complete** — 225 KB reconstructed of 3949 KB of `__TEXT` machine code (Tier 1+2, KB-weighted; Tier 3 AUTO / Tier 4 SKIP are complete by design).
- **Remaining**: 3725 KB of machine code (Tier 1: 2046 KB, Tier 2: 1678 KB).
- **Estimated time to finish**: ~10 months at the observed serial single-conductor pace (12.5 KB/day, calibrated on the M3 terrain wave). Parallelizing implementers across disjoint classes would reduce this roughly proportionally.
- **Estimated tokens to finish**: ~7.4B LLM tokens at the observed ~2M tokens/KB.
- The percentage is hard data; the time/token figures are order-of-magnitude planning extrapolations, not commitments.

Per-symbol states (honest coverage — a citation is not an implementation):

- **VERIFIED** — exercised by a `[PASS]` check of `opensc4_authentic_verify` (strongest).
- **IMPLEMENTED** — cited in a `.cpp`/`.mm` implementation unit, or part of a prefix-covered implemented class.
- **CITED** — address cited only in headers/tests: declared, not yet implemented.
- **TODO** — no citation, no code: the real remaining work.
- **AUTO** (Tier 3) — machine-verified GZCOM boilerplate / leaf accessors. **SKIP** (Tier 4) — superseded runtime, not reimplemented by design.

## Tier coverage

| Tier | Symbols | VERIFIED | IMPLEMENTED | CITED-only | TODO/AUTO/SKIP | KB total | KB remaining |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Tier 1: Core SC4 Simulation, World, Network, Automata & Terrain | 5521 | 98 | 324 | 5099 | 0 | 2247.6 | 2046.5 |
| Tier 2: UI Windows, 3D View/Render, Audio & Lua/Effects | 3924 | 2 | 80 | 3842 | 0 | 1701.7 | 1678.1 |
| Tier 3: Auto-Verified GZCOM Boilerplate & <=48B Leaf Accessors | 6932 | 0 | 0 | 0 | AUTO 6932 | 249.2 | 0.0 |
| Tier 4: Superseded cGZ/cRZ Framework, DX7 Shim, Thunks & STL | 26956 | 0 | 0 | 0 | SKIP 26956 | 5197.6 | 0.0 |

## Tier 1 remaining work (TODO + CITED-only), by class

| Class | VERIFIED | IMPL | CITED-only | TODO | KB left |
| :--- | ---: | ---: | ---: | ---: | ---: |
| `cSC4Automaton` | 0 | 1 | 113 | 0 | 32.5 |
| `cSC4CitySituationManager` | 0 | 0 | 98 | 0 | 57.4 |
| `cSC4LotDeveloper` | 1 | 3 | 98 | 0 | 35.4 |
| `cSC4TrafficSimulator` | 0 | 5 | 96 | 0 | 70.1 |
| `cSC4MySim` | 0 | 0 | 91 | 0 | 42.6 |
| `cSTETerrain` | 64 | 17 | 90 | 0 | 26.5 |
| `cSC4App` | 0 | 1 | 89 | 0 | 75.4 |
| `cSC4ResidentialSimulator` | 0 | 2 | 75 | 0 | 43.9 |
| `cSC4BuildingDevelopmentSimulator` | 0 | 4 | 73 | 0 | 24.4 |
| `cSC4NetworkOccupant` | 0 | 3 | 69 | 0 | 13.3 |
| `cSC4TrafficNetworkMap` | 0 | 2 | 69 | 0 | 23.4 |
| `cSC4AutomataManager` | 0 | 1 | 64 | 0 | 30.8 |
| `cSC4Watercraft` | 0 | 2 | 57 | 0 | 37.5 |
| `cSC4ZoneDeveloper` | 1 | 6 | 55 | 0 | 38.0 |
| `cSC4AutomataScriptSystem` | 0 | 0 | 52 | 0 | 31.0 |
| `cSC4MySimDispatch` | 0 | 1 | 51 | 0 | 25.1 |
| `cSC4PropManager` | 0 | 0 | 51 | 0 | 9.3 |
| `cSC4TrafficAutomaton` | 0 | 0 | 50 | 0 | 20.1 |
| `cSC4RegionalCity` | 0 | 0 | 49 | 0 | 19.4 |
| `cSC4Lot` | 0 | 4 | 48 | 0 | 14.1 |
| `cSC4PoliceSimulator` | 0 | 5 | 48 | 0 | 11.6 |
| `cSC4Vehicle` | 0 | 0 | 44 | 0 | 28.2 |
| `cSC4Helicopter` | 0 | 0 | 43 | 0 | 37.9 |
| `cSC4OccupantManager` | 0 | 2 | 43 | 0 | 13.8 |
| `cSC4PowerPoleOccupant` | 0 | 0 | 43 | 0 | 16.4 |
| `cSC4FloraSimulator` | 0 | 3 | 42 | 0 | 18.0 |
| `cSC4NeighborsSimulator` | 0 | 2 | 42 | 0 | 21.0 |
| `cSC4PathInfo` | 0 | 0 | 41 | 0 | 17.0 |
| `cSC4PowerSimulator` | 0 | 6 | 41 | 0 | 19.6 |
| `cSC4TractDeveloper` | 0 | 3 | 41 | 0 | 26.0 |
| `cSC4ConstructionOccupant` | 0 | 2 | 39 | 0 | 22.7 |
| `cSC4Airplane` | 0 | 0 | 38 | 0 | 33.8 |
| `cSC4PollutionSimulator` | 13 | 7 | 38 | 0 | 15.2 |
| `cSC4AudioScapeManager` | 0 | 0 | 37 | 0 | 8.8 |
| `cSC4TrafficAutomataManager` | 0 | 0 | 37 | 0 | 14.6 |
| `cSC4UndergroundOccupant` | 0 | 0 | 37 | 0 | 15.0 |
| `cSC4AudioAmbienceLayer` | 0 | 0 | 36 | 0 | 14.4 |
| `cSC4BudgetSimulator` | 2 | 7 | 36 | 0 | 11.4 |
| `cSC4AutomataControllerManager` | 0 | 0 | 35 | 0 | 13.4 |
| `cSC4NetworkConstructionCrew` | 0 | 0 | 35 | 0 | 14.9 |

## Tier 2 remaining work (TODO + CITED-only), by class

| Class | VERIFIED | IMPL | CITED-only | TODO | KB left |
| :--- | ---: | ---: | ---: | ---: | ---: |
| `cSC4ViewInputControlQuery` | 0 | 0 | 202 | 0 | 98.7 |
| `cSC4NetworkTool` | 0 | 3 | 140 | 0 | 111.0 |
| `cSC4View3DWin` | 0 | 0 | 134 | 0 | 77.9 |
| `cSC43DRender` | 0 | 0 | 91 | 0 | 39.9 |
| `cSC4WinLotConfigurationEditor` | 0 | 0 | 90 | 0 | 91.8 |
| `cSC4RenderProperties` | 0 | 0 | 85 | 0 | 25.7 |
| `cSTETerrainView3D` | 0 | 6 | 74 | 0 | 35.0 |
| `cHitTrackPlayer` | 0 | 0 | 62 | 0 | 23.5 |
| `cHitChannel` | 0 | 0 | 54 | 0 | 6.2 |
| `cSC4ModelMaker` | 0 | 0 | 54 | 0 | 32.7 |
| `SC4DrawContext` | 0 | 0 | 51 | 0 | 11.8 |
| `cSTEOverlayManager` | 0 | 0 | 50 | 0 | 19.3 |
| `cSC4EffectsManager` | 0 | 0 | 46 | 0 | 29.4 |
| `cHitMan` | 0 | 0 | 44 | 0 | 10.2 |
| `cSC4WinSnapshotAlbum` | 0 | 0 | 40 | 0 | 27.9 |
| `cSC4ParticlesEffect` | 0 | 0 | 39 | 0 | 29.3 |
| `cFreshPlayer` | 0 | 0 | 38 | 0 | 9.9 |
| `cSC4SubBudgetWindow` | 0 | 0 | 37 | 0 | 92.6 |
| `SC4ViewUtilities` | 0 | 1 | 36 | 0 | 14.5 |
| `cSLTokenRecord` | 0 | 0 | 36 | 0 | 4.9 |
| `cSTEVolcanoLava` | 0 | 0 | 36 | 0 | 11.0 |
| `cSC4WinMapView` | 0 | 0 | 34 | 0 | 34.0 |
| `cSC4WinRegionScreen` | 0 | 0 | 33 | 0 | 25.3 |
| `cSC4CameraControl` | 0 | 0 | 29 | 0 | 7.0 |
| `cSC4ViewInputControlPlayerDrive` | 0 | 0 | 28 | 0 | 14.2 |
| `nsISC4GameDataRegistry` | 0 | 0 | 27 | 0 | 5.9 |
| `cS3DModelInstanceAnimatedSprite` | 0 | 0 | 26 | 0 | 9.3 |
| `cSC4NetworkToolUI` | 0 | 0 | 26 | 0 | 9.1 |
| `cSC4PowerLineTool` | 0 | 0 | 26 | 0 | 13.9 |
| `cSC424HourClock` | 0 | 0 | 25 | 0 | 4.5 |
| `cSLTokenSerialRecordIO_Text` | 0 | 0 | 25 | 0 | 3.9 |
| `cSC4MySimWinProc` | 0 | 0 | 24 | 0 | 23.3 |
| `cSC4ViewInputControlPlaceLot` | 0 | 0 | 24 | 0 | 24.8 |
| `cSC4WinAnimationRecorder` | 0 | 0 | 24 | 0 | 15.6 |
| `cSC4WinMiniMap` | 0 | 0 | 24 | 0 | 9.8 |
| `SC4GeometryUtilities` | 0 | 0 | 23 | 0 | 4.4 |
| `cSC4BudgetUIWinProc` | 0 | 0 | 23 | 0 | 20.1 |
| `cSC4EffectsCollection` | 0 | 0 | 23 | 0 | 9.4 |
| `cSC4TutorialUIWinProc` | 0 | 0 | 23 | 0 | 17.7 |
| `SCLuaHelp` | 0 | 0 | 22 | 0 | 9.6 |

## Next actions

1. `python3 tools/sc4_decomp_audit.py --next "" 25` — next TODO symbols.
2. `python3 tools/sc4_decomp_audit.py --workpack <Class>` — workpack for a class.
3. `--sync-registry` after citing new addresses; rerun verify and regenerate this file.

