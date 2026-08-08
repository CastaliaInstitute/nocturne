# Deploying the Nocturne PWA to Cloudflare

The Nocturne PWA (`pwa/`) is deployed to Cloudflare as an assets-only Worker
named `nocturne-pwa`, configured by `wrangler.toml` at the repository root.
The production host is `nocturne.castalia.institute`, attached as a custom
domain route in `wrangler.toml` — DNS and TLS are auto-provisioned because
`castalia.institute` is a zone in the same Cloudflare account (the account ID
is inline in `wrangler.toml`).

## The one missing piece: the API token secret

CI deploys fail today with:

> `CLOUDFLARE_API_TOKEN secret is not configured for this repository.`

To fix it:

1. In the Cloudflare dashboard, go to *My Profile → API Tokens* and create a
   token from the **Edit Cloudflare Workers** template, scoped to the account
   (and the `castalia.institute` zone for custom-domain management).
2. In the GitHub repository settings, add it as an Actions secret named
   `CLOUDFLARE_API_TOKEN`.

No other secret is needed — the account ID is committed in `wrangler.toml`.

## Pipelines

- `.github/workflows/deploy-cloudflare.yml` — smoke-tests the bundle on every
  PR touching the PWA; on pushes to `main` it deploys via `wrangler@4`. If the
  secret is missing it skips the deploy with a warning instead of failing, so
  CI stays green until the secret exists.
- `.github/workflows/deploy-pwa.yml` (on the `agent/complete-foundation`
  branch) — deploys that branch directly and verifies the live site serves the
  deployed commit's assets.

Manual deploy from a machine with a token:

```bash
CLOUDFLARE_API_TOKEN=... npx wrangler@4 deploy
```

## Verifying a deploy

- `https://nocturne.castalia.institute/` loads the app
- `https://nocturne.castalia.institute/manifest.webmanifest` returns the
  manifest with `Content-Type: application/manifest+json`
- Response headers include `Strict-Transport-Security` and
  `Permissions-Policy: bluetooth=(self), ...` (from `pwa/_headers`; Web
  Bluetooth requires a secure context and the policy keeps it scoped to the
  app's own origin)
- `service-worker.js` is served with `Cache-Control: no-cache` so client
  updates roll out promptly

## Local preview

```bash
npx wrangler@4 dev
```

This serves the same bundle Cloudflare will serve, including `_headers`
processing, on a local port.
