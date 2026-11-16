# OpenSC4

An independent C++20 reimplementation of **SimCity 4 Deluxe**, following the
game-preservation tradition of OpenTTD, OpenRCT2 and OpenMW.

**Development preview: not a feature-complete replacement for the original game.**
The immediate goal is Authentic: reproduce the original simulation, save formats,
rendering and interactions, including numerical quirks. Enhanced development is
frozen until Authentic reaches the feature-complete M6 gate.

OpenSC4's original code and documentation are available under the
[MIT licence](LICENSE). Bring your own lawful installation for original assets
and original-game comparisons; the licence does not cover EA/Maxis material.
See [licensing boundaries](NOTICE.md).

## Why this project

Long-lived simulation games contain years of player creations and modding work.
A maintainable replacement engine can preserve access to those worlds and make
their behavior inspectable on modern systems. OpenTTD, OpenRCT2 and OpenMW show
that sustained open-source engine recreation is a useful model. Their maturity
is an aspiration, not a claim about OpenSC4's current readiness.

This is **rslmart/OpenSC4**, a separate C++ project. It is not the older
Godot-based OpenSC4-org project or the separate Rust project at tloriato/OpenSC4.
Read the [project context and earlier attempts](docs/PROJECT_CONTEXT.md) for
primary sources, the older project's observed inactivity, and limits on what
can be concluded about it.

## Current evidence and remaining work

The reviewed weather-coordinate component, committed as `aa239ea`, has retained
normal-build evidence of **871 suite checks passing, zero failures**, and
**417 city saves / 2,185,774 applicable records roundtripping byte-exactly**.
These are recorded results for the tested component revision, not a claim that
every current checkout, platform or gameplay path has been verified.

The public repository begins with a fresh source snapshot. Historical revision
identifiers in retained records refer to the earlier development history. Fresh
publication checks are recorded in [PUBLICATION.md](docs/verification/PUBLICATION.md).

- [Integration record and raw-log references](authentic/recon/weather_batch/INTEGRATION_HANDOFF.md)
- [Independent semantic/integration review](authentic/recon/weather_batch/INTEGRATION_REVIEW.md)
- [Authenticity contract and contributor workflow](AGENTS.md)
- [Milestones and acceptance ladder](ROADMAP.md)
- [Generated symbol backlog](authentic/STATUS.md)

The public roadmap and selected evidence records describe the published scope.
Private orchestration and campaign records are excluded from this source snapshot.

Save interoperability has a bounded original-game acceptance record. Whole-city
evolution parity, complete simulator scheduling, rendering/interaction parity and
extended play remain unfinished. The weather batch's blind reconstruction and
live original-game comparison are also outstanding. Implemented, suite-verified,
independently reviewed and live-proven remain separate claims.

## Build Authentic

Requirements: CMake 3.20 or newer and a C++20 compiler. The macOS frontend also
uses the Apple SDK, Cocoa and OpenGL. Other platform targets exist but are not
covered by the macOS verification evidence above.

```sh
cmake -S . -B build -DOPENSC4_BUILD_ENHANCED=OFF -DOPENSC4_BUILD_TESTS=ON
cmake --build build
./build/authentic/opensc4_authentic_verify
```

The verifier discovers original data through the repository's existing data
loader. Full shipped-city coverage requires a readable supported installation;
without it, inspect skipped/absent-data output rather than claiming the 417-city
gate. Original-game live checks are a separate, controlled workflow documented
in [the oracle README](authentic/tools/oracle/README.md).

Build commands are contributor instructions; no fresh platform certification or
new verification result is implied by this README.

## Contribute and support

Start with [CONTRIBUTING.md](CONTRIBUTING.md). Useful contributions include
bounded behavioral reconstruction, discriminating regression cases, independent
review, compatibility investigations and documentation. Use repository issues
and pull requests for public collaboration.

We are preparing an application to OpenAI's Codex Open Source Fund to support
implementation and independent verification. The [application draft](docs/CODEX_FUND_APPLICATION.md)
contains the proposed scope and evidence links. **No award or OpenAI endorsement
is claimed.**

SimCity and SimCity 4 are trademarks of Electronic Arts. This project is not
affiliated with or endorsed by Electronic Arts, Maxis, OpenTTD, OpenRCT2 or OpenMW.
