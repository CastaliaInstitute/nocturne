# Phase 9 — Privacy, Security, and Trust

## Security posture summary

Nocturne targets local-first operation by default:

- Core adaptation data and session context are stored in browser local storage only for this scaffold.
- Network calls are restricted to explicit allowlisted API hosts.
- PWA features expose explicit user-facing consent state transitions before enabling data access or BLE pairing.

## Threat model and control matrix

| Threat | Impact | Control |
| --- | --- | --- |
| Unauthorized repository access from stolen credentials | Sensitive repository data exposure | Session-scoped credentials are required; Castalia/token exchange is explicit and separate from BLE. |
| Consent abuse (silent permissions) | Unexpected data collection or sensor pairing | Permission state machine blocks repo/BLE features until checked consent flags are present. |
| Unbounded exfiltration | Privacy leakage to unknown endpoints | Runtime network allowlist check (`ALLOWED_REMOTE_HOSTS`) in the PWA client and policy audit validation. |
| Offline regression or data loss | User loses context/state unexpectedly | Session and snapshot storage remain in `localStorage`; explicit delete flow requires confirmation. |
| Debugging info leaks | Secrets accidentally logged | All sensitive events are routed through structured `logLine`, without raw token logging. |

## Policy enforcement points

- Session state: `nocturne-pwa-session` is only read/written through explicit save/clear helpers.
- Permissions: repo and BLE controls use `requirePermission()` checks before action.
- Network allowlist: `isAllowedNetworkTarget()` rejects calls outside allowed hosts.
- Storage minimization: offline persistence is scoped to required keys only.
- Consent transitions: `permissionState` is recomputed after all consent/login/session mutations.

## Penetration review checklist

Perform this review quarterly:

- Validate `localStorage` persistence keys against approved list.
- Attempt to trigger action buttons with missing consent and confirm blocked state.
- Confirm network traffic stays limited to approved hosts when user interactions are exercised.
- Validate delete-data flow requires explicit confirmation and clears local keys.
- Confirm exported local package contains only approved keys and non-sensitive metadata.
- Check service worker and offline snapshot behavior does not persist token-bearing payloads.

## Periodic security and trust review

- Quarterly update of this matrix against deployment environment changes.
- Align `scripts/privacy-audit.mjs` with any added remote endpoints or permission states.
- Log review after any phase-5 to phase-9 implementation changes to prevent regression of consent gating.

