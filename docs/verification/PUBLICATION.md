# Public source snapshot checks — 2026-11-16 (Sydney)

This repository starts with a fresh source snapshot. Earlier development history
and private strategy, orchestration and campaign records are excluded. Selected
technical evidence and required native diagnostic/verification source fixtures
are retained. A private session locator was removed from the selected independent
review record; its semantic findings were retained.

## Freshly exercised checks

- CMake configure: exit 0, Authentic enabled and Enhanced disabled.
- Normal C++ build: exit 0. Existing unused-variable/field and macro-redefinition
  warnings are preserved in the log.
- Verifier: **871 passed, 0 failed**, exit 0.
- Save corpus: **417 cities**, 49,096 subfiles, 146 type IDs;
  **2,185,774 / 2,185,774 applicable records byte-exact**.
- Eleven weather integration source hashes match the retained handoff.
- Original runtime C++ implementations/headers/test sources were preserved;
  publication preparation changed documentation and excluded private artifacts.
- Targeted scans of the publication files/paths found no private-project or
  orchestration-application references, known credential patterns, proprietary
  game-container/executable signatures or compiled native artifacts. These are
  bounded checks, not a complete provenance/legal or secret audit.
- Public overview/application local links and relative native source includes
  were checked. Whitespace checks cover newly authored documentation; raw logs
  and inherited generated files retain their original whitespace. The source-root build output directory is excluded from Git.

## Recorded failed first run

The first verifier run reported **862 passed / 9 failed**. It was built outside
the checkout, but nine save tests use a source-root `build/` output directory.
That directory did not exist in the fresh export. Creating the expected ignored
output directory, without changing the implementation or assertions, produced
the successful full rerun above. This limitation matters to contributors using
out-of-tree build directories; the README's source-root build layout supplies
the expected directory.

## Logs

- [Configure](publication-configure.log)
- [Build](publication-build.log)
- [Successful verifier](publication-verify.log)
- [First verifier run, missing output directory](publication-verify-missing-output-directory.log)

No fresh independent binary semantic review, original-game live call, blind
reconstruction, Phase B replacement or full gameplay completion is claimed by
these publication checks. Retained component reviews retain their original
scope and qualifications.
