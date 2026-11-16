# Codex Open Source Fund — application draft

Prepared **2026-11-16** for maintainer review and manual submission.
**Not submitted; no award, partnership or endorsement is claimed.**

## Programme and form

- [Official application](https://openai.com/form/codex-open-source-fund/)
- [Related Codex for Open Source programme](https://developers.openai.com/community/codex-for-oss)

The fund form currently advertises rolling review and grants up to $25,000 in
API credits. It asks for applicant details, project name/description, repository,
collaborators and use of credits. It does not list a VC referral as a required
field. The related programme also describes subscription benefits; these are
not assumed to be a second independent API-credit award.

README, contribution guidance and this draft help reviewers assess the project;
they are preparation materials, not additional requirements stated by the form.

## Applicant fields

- **Name:** Russell Martin — confirm your preferred application name.
- **Email:** enter the account/contact address you want OpenAI to use.
- **Personal GitHub:** https://github.com/rslmart
- **LinkedIn:** optional; enter your own URL if desired.
- **Project:** OpenSC4 (rslmart/OpenSC4, independent C++20 project)
- **Repository:** https://github.com/rslmart/OpenSC4
- **Collaborators:** enter actual human collaborators and roles, if any.
  AI worker/reviewer sessions are part of the workflow, not additional human
  maintainers. No contributor count is asserted by this draft.

## Brief description — ready to paste

OpenSC4 is an independent, MIT-licensed C++20 reimplementation of SimCity 4
Deluxe. Its goal is to preserve the original simulation, numerical behavior,
save compatibility and player-created cities on modern platforms. Users supply
their own original game assets.

OpenTTD, OpenRCT2 and OpenMW are representative precedents for preserving games
through maintainable replacement engines. An earlier Godot-based OpenSC4 project
also pursued this goal, but its public repository appears inactive: GitHub reports
its last push in March 2024. We have not verified why work stopped. Our C++ project
is independent of that effort and of a separate recent Rust OpenSC4 project.

Development uses address-cited behavioral reconstruction, focused Codex tasks,
independent review, boundary tests and controlled comparisons with the original
game. The reviewed weather-component revision has retained evidence of 871 suite
checks passing and byte-exact roundtrips across 417 cities and 2,185,774 applicable
records. These are bounded results: full simulation, visual and extended-play
parity remain unfinished. Credits would support completing and independently
verifying further milestones rather than generating code without acceptance
evidence.

## How would you use API credits? — ready to paste

We would use OpenAI API credits to advance OpenSC4's Authentic implementation
through focused implementation, independent semantic review, regression-test
development and integration. The work targets faithful simulation and save
compatibility, with rendering and interactions following the project roadmap.

Our workflow deliberately separates implementation from independent assurance.
New simulation behavior must preserve float operation order, branch boundaries
and RNG behavior. Changes are checked through reproducible builds, discriminating
tests, byte-exact save roundtrips and controlled original-game comparisons where
reachable. Passing local tests alone is not treated as gameplay parity.

We propose staged use of up to $25,000 in API credits, subject to the programme's
award and expiry terms. Initial work would close outstanding proof on bounded
simulation components and then progress through independently owned slices of
the existing roadmap. We would measure accepted deliverables, lifecycle token
use, defects detected, and review/live-test waiting time before expanding
parallel work. No particular speedup or completion date is promised.

We would publish milestone summaries linking source revisions, independently
reviewed changes, actual verification commands/results and remaining gates.
Funding would increase implementation and verification capacity while keeping
the compatibility requirements intact.

## Optional additional information — ready to paste

The project is maintained by Russell Martin and already uses Codex in a focused,
tiered development workflow. It is an opportunity to evaluate AI-assisted game
preservation against an unusually concrete reference: original executable
behavior and an existing save corpus. The repository includes evidence and
review records, including failures and unresolved results. Original commercial
assets remain outside the source distribution.

We are seeking support for a useful open-source engine and an honest account of
which AI-assisted practices produce accepted, reproducible work. We do not claim
affiliation with Electronic Arts, Maxis, the precedent projects or OpenAI.

## Proposed credit allocation — planning only

| Activity | Proposed ceiling |
| --- | ---: |
| Focused implementation and integration fixes | $10,000 |
| Independent semantic review and blind reconstruction | $8,000 |
| Boundary regression and compatibility checks | $4,000 |
| Reproduction, release documentation and evidence reporting | $3,000 |
| **Total requested support** | **Up to $25,000** |

These are proposed allocations, not incurred costs, awarded funds or a token
forecast. Actual model mix, cached-input usage, pricing, programme restrictions
and accepted milestone results determine expenditure. Reassess after initial
accepted slices; do not convert subscription allowance percentages into tokens.

## Evidence links and claim boundaries

- [Public-facing overview](../README.md), [MIT licence](../LICENSE) and
  [licensing boundaries](../NOTICE.md).
- [Primary sources for precedents and earlier attempts](PROJECT_CONTEXT.md),
  checked 2026-11-16.
- [Weather integration record](../authentic/recon/weather_batch/INTEGRATION_HANDOFF.md)
  and [independent review](../authentic/recon/weather_batch/INTEGRATION_REVIEW.md).
  Reviewed overlay was based on `02472c8`; accepted component was committed as
  `aa239ea` in the pre-publication development history. This public repository
  starts from a fresh source snapshot; those historical identifiers are retained
  as provenance labels, not claimed public commit links. The records distinguish normal-suite results, leaf checks and missing
  blind/live proof. Fresh public-snapshot checks are recorded separately in
  [PUBLICATION.md](verification/PUBLICATION.md).
- [Roadmap](../ROADMAP.md), [contribution guide](../CONTRIBUTING.md) and
  [agent contract](../AGENTS.md). The public roadmap and selected
  evidence records describe the published scope; private campaign and orchestration
  records are excluded.

## Before submitting

- Confirm applicant/contact information and actual collaborator roles.
- Publish these approved preparation files with the intended source revision.
- Verify the published repository is accessible to reviewers without signing
  in. Publication checks are recorded in docs/verification/PUBLICATION.md.
  The public repository uses fresh history; private development history is not
  included in the public source snapshot.
- Review the files intended for publication, third-party provenance/notices and
  original-asset exclusions. Adding MIT does not relicense third-party material
  or establish that a complete provenance audit has passed.
- Recheck the linked programme and form; adapt text to any length limits and
  current terms. No mandatory pitch deck, stars threshold or fixed response time
  was established from the fund form.
- Submit manually after reviewing the form's terms. No account creation, remote
  visibility change or submission is authorized by this local preparation step.
