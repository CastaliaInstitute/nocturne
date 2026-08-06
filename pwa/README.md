# Nocturne PWA

Purpose:
- Provide a Castalia-branded web control surface at `nocturne.castalia.institute`
- Let users authenticate to Castalia and connect to a per-user repo `castalia-{username}`
- Expose Web BLE access for ring/device control
- Keep a baseline offline runtime path

## Current status

This is a minimal, bootstrap implementation with:
- Login form for scoped identity/session storage
- GitHub repo discovery for `castalia-{username}`
- Consent-gated repository access and BLE/device pairing
- Repository workspace file + commit browsing
- Optional Castalia token-exchange attempt
- Web Bluetooth discovery and connection workflow
- Offline snapshot dump and service-worker bootstrap

## Security and auth note

This scaffold uses an explicit token field for GitHub API access and local storage
for convenience. Production should use backend token exchange/OAuth and secure
storage policies.

## Deployment and smoke checks

- Host at `https://nocturne.castalia.institute` with HTTPS.
- Smoke checklist:
  - Open app and click **Register Service Worker**.
  - Turn off network and reload to validate cached shell availability.
  - Sign in with test account, enable consent, and resolve repo.
  - Validate **List workspace files** and **Show recent changes** return content.


## Run

- Serve this folder as static web content over HTTPS (required for Web Bluetooth).
- For local testing:

```bash
python -m http.server --directory pwa 8000
```

Then open `https://localhost:8000` with a modern Chromium-based browser for Web
Bluetooth support.

## Domain strategy

- DNS: `nocturne.castalia.institute`
- Static hosting target should serve:
  - `pwa/index.html`
  - `pwa/manifest.webmanifest`
  - `pwa/service-worker.js`
  - `pwa/app.js`
  - `pwa/styles.css`

## Missing assets

- `icon-192.png` and `icon-512.png` placeholders are required before production.
