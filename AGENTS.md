# OpenSC4 contributor and agent contract

This public source snapshot follows the Authentic-first development contract.
Read README.md, CONTRIBUTING.md and ROADMAP.md before making changes.

- Authentic recreates SimCity 4 Deluxe behavior, numbers, saves and rendering.
- Enhanced is frozen until Authentic reaches feature-complete milestone M6.
- Original game installations are read-only. Never commit game executables,
  libraries, archives, city saves, vendor source, assets or credentials.
- Cite machine-code addresses for reconstructed behavioral claims. Preserve float
  operation order, branch structure, thresholds, RNG order and serialization order.
- Mark unsupported procedural behavior as a clean-room fallback.
- Use fresh binary evidence to settle semantic disputes. Agreement between code,
  specifications and tests does not replace independent checks against the original.
- Build with CMake and run the verifier. Simulation tests must cover boundaries;
  serialization changes retain byte-exact coverage across all 417 shipped cities.
- Independent semantic review is required for float-heavy or state-coupled code.
  Implementers cannot provide their own independent assurance.
- Preserve blind reconstruction and reachable live original-game proof gates.
  A green suite alone does not establish gameplay parity.
- Sync the address registry and regenerate authentic/STATUS.md when applicable.
- Keep implementation, suite verification, independent review and live proof
  claims separate, with tested source identity and evidence references.
- Work on bounded, owned tasks. Protect unrelated local work and use scoped commits.
- Stop after three consecutive failures of the same implementation approach;
  return to evidence or split the task instead of repeating it unchanged.

The retained review and verification records are evidence for their documented
revisions and domains, not certification of every public checkout or platform.
