# Project context: precedents and earlier OpenSC4 efforts

Sources checked **2026-11-16**. These projects demonstrate relevant approaches
to preserving games through replacement engines. They do not establish legal
clearance, equivalent maturity, transferred code, affiliation or endorsement for
this repository. OpenSC4's own acceptance evidence remains separate.

## Representative precedents

| Project | Primary reference | Relevance to OpenSC4 |
| --- | --- | --- |
| OpenTTD | [Official about page](https://www.openttd.org/about) | An open-source simulation game based on Transport Tycoon Deluxe, aiming to mimic the original while extending it. Its long-running compatibility-first model is this project's stated inspiration. Its code is GPL v2; this repository's MIT choice does not permit copying it without considering its licence. |
| OpenRCT2 | [Official project site](https://openrct2.io/) | An open-source reimplementation of RollerCoaster Tycoon 2. It is a relevant example of recreating a complex management simulation and maintaining a usable community project. |
| OpenMW | [Official project repository/mirror](https://github.com/OpenMW/openmw) | A replacement engine supporting Morrowind, requiring users to own the game. It illustrates separating an independently maintained engine from original commercial data. Its GPL-licensed code is not incorporated or relicensed by this comparison. |

These are examples of project direction, not evidence that OpenSC4 can meet a
particular schedule or reproduce their success. OpenSC4's distinctive proposed
contribution is faithful SC4 behavior with traceable binary evidence, independent
review and save/runtime comparisons. Those mechanisms still need to close the
remaining full-game gates.

## The older Godot OpenSC4 attempt

[OpenSC4-org/OpenSC4](https://github.com/OpenSC4-org/OpenSC4) describes an
open-source SC4 recreation using Godot. Its
[README](https://github.com/OpenSC4-org/OpenSC4/blob/main/README.md) documents
loading an original installation into a region view.

The public [GitHub repository API](https://api.github.com/repos/OpenSC4-org/OpenSC4)
reported `pushed_at=2024-03-29T19:53:56Z` and `archived=false` when checked on
2026-11-16. This supports describing the repository as **apparently inactive
since its last reported push**. Push metadata is not a definitive development or
release history. No maintainer statement explaining abandonment or failure was
verified in this check.

For the funding application, acknowledge it as an earlier attempt whose public
record does not demonstrate the completed replacement we are pursuing. Do not
attribute its outcome to poor management, lack of AI, technical impossibility or
any other unverified cause. This repository is not presented as its continuation
or as having inherited its code, contributors or community.

## A separate recent Rust project

[tloriato/OpenSC4](https://github.com/tloriato/OpenSC4) is another, distinct SC4
reimplementation. Its [README](https://github.com/tloriato/OpenSC4/blob/main/README.md)
describes a Rust core with a Windows game plugin and shadow-comparison workflows.
Those are its authors' reported results; they have not been reproduced here.
The repository API reported creation on 2026-10-12 and last push on 2026-10-29
when checked. It must not be conflated with the older Godot attempt or described
as a failed project on the basis of that older history.

Its published GPL-3.0-or-later licensing and independent provenance need to be
considered before any future reuse. No code import, collaboration, comparative
quality claim or runtime comparison is part of this documentation change.

## What this application should claim

Identify the applicant's project as **rslmart/OpenSC4 (C++20)**. Explain the
preservation goal, disclose the current pre-release state, cite tested revisions
and existing verification records, and request support for bounded next steps.
Do not claim adoption, downloads, active contributors, completed gameplay parity
or a measured AI speedup without supporting records. Earlier projects make the
case for sustained verification and maintainership, not a guarantee of success.
