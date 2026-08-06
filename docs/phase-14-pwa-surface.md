# Phase 14 — Nocturne PWA Surface

## Implemented in this pass

- Consent-driven onboarding flow for Castalia data access + BLE access.
- Castalia authentication flow scaffold:
  - username/token capture
  - optional token exchange attempt with `api.castalia.institute` and fallback to PAT mode
  - session token persistence in local storage (for this scaffold only)
- Repository access UI:
  - discover repo
  - open repo in GitHub
  - list workspace files
  - show recent commits
- WebBLE pairing controls with explicit consent gate.
- Offline UX hardening:
  - service worker registration
  - offline snapshot generation
- Local data controls:
  - export local cache/session snapshot
  - delete local data with explicit confirmation
- Policy and privacy controls:
  - consent-state machine with explicit permission transitions
  - network allowlist enforcement for all outbound calls
- Validation:
  - privacy audit script (`scripts/privacy-audit.mjs`) now validates policy gates, storage minimization, and lint-like checks
- Deployment readiness checks:
  - added CI workflow `pwa-smoke` with static artifact validation and optional host check.

## Not yet implemented

- Real `nocturne.castalia.institute` service-account provisioning and DNS configuration.
- Hosted backend for token exchange and permission scoping.
- Repository file browsing + activity UI is currently read-only metadata only.
