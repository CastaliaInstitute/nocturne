# Contributing to Nocturne

Thanks for helping build Nocturne.

## Expectations

- Keep changes small and explainable.
- Respect deterministic behavior: do not alter behavior without updating tests and
  documenting assumptions.
- Document any new adaptation policy, safety bound, or context signal path.
- Prefer local-first and privacy-preserving designs by default.
- Prefer explicit over implicit behavior in core modules.

## Repo workflow

- `main` is the release branch.
- Pull requests should include:
  - rationale for the change
  - tests run (if any)
  - safety/privacy impact
- Keep commits focused to a single intent.

## Style

- Use ASCII unless Unicode is essential.
- Use clear module boundaries matching Context -> Fusion -> Scheduler -> Rendering.
- Favor deterministic implementations and bounded signal blending.

## Questions

Open an issue before proposing protocol-breaking changes to the acoustic intent
vector or adaptation policies.
