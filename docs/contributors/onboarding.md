# Nocturne Contributor & Integrator Onboarding

## What to clone

- Core library in `core/`
- PWA in `pwa/`
- Tooling in `scripts/`

## First runbook

1. Read phase docs relevant to your area:
   - Core: `docs/phase-1-context-models.md`
   - Fusion: `docs/phase-3-context-fusion.md`
   - Scheduling: `docs/phase-4-scheduler.md`
   - PWA: `docs/phase-14-pwa-surface.md`
2. Run the baseline checks:
   - `./scripts/run-core-tests.sh`
   - `./scripts/run-core-benchmark.sh`
   - `./scripts/run-core-profiles.sh`
3. Open `pwa/index.html` with a local server (or any static host).

## Contribution conventions

- Keep changes deterministic first.
- Maintain explainability logs for scheduling-affecting changes.
- Add/extend tests adjacent to module changes.

## Review checklist

- Does it preserve local-first behavior?
- Are safety bounds still enforced?
- Are new assumptions documented in `docs/`?
- Is behavior reproducible with fixed seeds?
