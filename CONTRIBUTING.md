# Contributing to OpenSC4

OpenSC4 is in active development. Read [README.md](README.md),
[ROADMAP.md](ROADMAP.md) and the existing [AGENTS.md contract](AGENTS.md) before
changing behavior. That contract and the canonical plans own development and
acceptance rules; this guide summarizes them without replacing them.

## Scope and evidence

- Work in `authentic/`; Enhanced is frozen through Authentic milestone M6.
- Pick a bounded ticket and agree ownership before editing shared files.
- Cite the original machine-code addresses behind reconstructed behavior.
- Preserve float operation order, branches, thresholds, RNG order and serializer
  field order. Record oddities rather than silently improving them.
- Mark procedural behavior that lacks original evidence as a clean-room fallback.
- Use fresh binary evidence for disputes; test/spec/implementation agreement alone
  does not establish correctness.

## Verification and review

Run the build and verifier, and preserve meaningful failure output. Simulation
checks need boundary cases. Serialization changes must retain all-417 byte-exact
coverage on a supported original installation. Follow AGENTS.md for registry
sync, STATUS regeneration, independent review, blind reconstruction and reachable
live proof. An implementer cannot supply their own independent assurance.

In a pull request, state the tested source revision, changes, evidence addresses,
commands/results, skipped or failed gates and remaining limitations. Distinguish
implementation, suite verification, independent review and live proof. Protect
unrelated working-tree changes and use explicit commit paths.

## Licensing and materials

Submit only material you have the right to contribute. Original contributions
are submitted under the repository's MIT licence; retain applicable third-party
notices and identify any reused material and its licence. Do not copy code from
another project's repository merely because it is publicly readable.

Never commit game executables, archives, city saves, textures, audio, vendor
source files or private credentials. Original installations stay read-only.
Share reproducible steps and appropriately scoped evidence instead of assets.
See [NOTICE.md](NOTICE.md).

## Reporting problems

Use repository issues for ordinary bugs and feature discussions. Include platform,
source revision, reproduction steps and the relevant bounded logs. Remove private
paths and credentials where unnecessary. Do not upload proprietary game files.
No published security-response service or response-time guarantee is established.
