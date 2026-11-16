# OpenSC4 technical overview

OpenSC4 is an independent C++20 replacement engine for SimCity 4 Deluxe.
The current implementation goal is Authentic parity; see ROADMAP.md.

## Source layout

- `authentic/include/opensc4/`: core interfaces and reconstructed records.
- `authentic/src/`: data formats, serialization, simulation and rendering.
- `authentic/tests/`: boundary, compatibility and component checks.
- `authentic/tools/`: binary inspection, verification and controlled live-oracle tools.
- `authentic/recon/`: bounded investigation, review and verification records.

## Compatibility and verification

The engine reads original DBPF archives and city saves from a user-provided
installation. Reconstructed behavior is tied to original machine-code addresses.
Byte-exact save roundtrips, independently reviewed arithmetic and controlled
original-game comparisons provide distinct verification boundaries. Full city
evolution, rendering and extended-play parity remain unfinished.

Procedural fallbacks are explicitly distinguished from original behavior.
Platform-specific interfaces are separate from portable core components, but
platform target presence is not evidence of equivalent verification coverage.

Read README.md for build instructions and current bounded results, CONTRIBUTING.md
for the workflow, NOTICE.md for reference-material boundaries, and the existing
component records for exact tested revisions and missing gates.
