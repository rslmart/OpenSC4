# OpenSC4 roadmap — Authentic first

This roadmap describes game-recreation milestones. Status is bounded by the
linked verification evidence; it is not a percentage-complete claim.

| Milestone | Outcome | Current boundary |
| --- | --- | --- |
| M1 Foundation | Original data readers and save serialization | Retained all-417 byte-exact verification evidence; preserve it on changes. |
| M2 Interoperability | Original game accepts OpenSC4-written changes | Bounded original-game interoperability accepted; see retained evidence. |
| M3 Live simulation | Restore and connect the original simulation systems | In progress; isolated implemented components do not establish full scheduling or loaded-state parity. |
| M4 Evolution parity | Match original city evolution over day, month and year horizons | Complete integrated proof remains outstanding. |
| M5 Interaction and rendering | Match tools, UI and city presentation | Complete interaction and visual parity remain outstanding. |
| M6 Feature-complete Authentic | Region play, neighbors, advisors, automata, disasters, audio, tutorials and extended play | Open; preserve all earlier gates while closing end-to-end feature coverage. |

Enhanced remains frozen until M6. This public roadmap contains no commitments
about subsequent products or integration plans.

## Acceptance ladder

1. Reproducible build and meaningful boundary tests.
2. Address/constant/branch checks against original evidence.
3. Byte-exact roundtrips across all 417 shipped city saves.
4. Independently reviewed behavior, including blind reconstruction where required.
5. Controlled original-game comparison and replacement proof where reachable.
6. Integrated evolution, interaction, rendering and extended-play comparisons.

Implemented, suite-verified, independently reviewed and live-proven remain
separate claims. README.md links the current component evidence;
`authentic/STATUS.md` is a generated symbol backlog, not a full-game completion score.
