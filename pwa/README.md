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
- Web Bluetooth discovery and connection workflow
- Offline snapshot dump and service-worker bootstrap

## Security and auth note

This scaffold uses an explicit token field for GitHub API access and local storage
for convenience. Production should use backend token exchange/OAuth and secure
storage policies.

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
- Hosting: Cloudflare assets-only Worker configured by `wrangler.jsonc` at the
  repository root, serving this folder (including `_headers` for security and
  caching policy). See `docs/deployment/cloudflare.md` for account setup,
  CI secrets, and custom-domain attachment.

## Assets

- `icon-192.png` and `icon-512.png` are generated placeholder crescent icons;
  replace with final branding before public launch.
