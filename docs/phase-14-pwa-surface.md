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
- Deployment readiness checks:
  - added CI workflow `pwa-smoke` with static artifact validation and optional host check.

## Not yet implemented

- Real `nocturne.castalia.institute` service-account provisioning and DNS configuration.
- Hosted backend for token exchange and permission scoping.
- Repository file browsing + activity UI is currently read-only metadata only.
