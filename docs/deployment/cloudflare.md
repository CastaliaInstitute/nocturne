# Deploying the Nocturne PWA to Cloudflare

The Nocturne PWA (`pwa/`) is deployed to Cloudflare as an assets-only Worker,
configured by `wrangler.jsonc` at the repository root. The production host is
`nocturne.castalia.institute`.

## What the repository already provides

- `wrangler.jsonc` — Worker named `nocturne` serving `pwa/` as static assets
- `pwa/_headers` — security headers (HSTS, nosniff, frame denial), a
  Web Bluetooth permissions policy, and `no-cache` on the service worker so
  updates roll out promptly
- `pwa/icon-192.png`, `pwa/icon-512.png` — PWA icons referenced by the manifest
- `scripts/pwa-smoke.mjs` — pre-deploy consistency check for the static bundle
- `.github/workflows/deploy-cloudflare.yml` — CI pipeline: smoke test on every
  PR touching the PWA, automatic deploy on pushes to `main`

## One-time Cloudflare account setup

These steps happen in the Cloudflare dashboard and cannot be done from the
repository:

1. **Add the zone.** Ensure `castalia.institute` is an active zone on the
   Cloudflare account (or delegate just the `nocturne` subdomain via a CNAME if
   the zone lives elsewhere).
2. **Create an API token.** In *My Profile → API Tokens*, create a token from
   the **Edit Cloudflare Workers** template, scoped to the account (and zone,
   for custom-domain attachment).
3. **Find the account ID.** Shown on the right side of any zone's *Overview*
   page, or under *Workers & Pages*.
4. **Add GitHub secrets.** In the repository settings, add:
   - `CLOUDFLARE_API_TOKEN`
   - `CLOUDFLARE_ACCOUNT_ID`

   Until both secrets exist, the deploy job runs but skips the Wrangler step
   with a notice, so CI stays green.
5. **First deploy.** Either push to `main` (with secrets set) or deploy
   locally:

   ```bash
   npx wrangler deploy
   ```

6. **Attach the custom domain.** In *Workers & Pages → nocturne → Settings →
   Domains & Routes*, add `nocturne.castalia.institute` as a **Custom Domain**.
   Cloudflare creates the DNS record and certificate automatically when the
   zone is on the same account.

## Verifying a deploy

- `https://nocturne.castalia.institute/` loads the control surface
- `https://nocturne.castalia.institute/manifest.webmanifest` returns the
  manifest with `Content-Type: application/manifest+json`
- Response headers include `Strict-Transport-Security` and
  `Permissions-Policy: bluetooth=(self), ...` (Web Bluetooth requires a secure
  context; the permissions policy keeps it scoped to the app's own origin)
- The service worker registers from the *Offline and Explainability* panel and
  `service-worker.js` is served with `Cache-Control: no-cache`

## Local preview

```bash
npx wrangler dev
```

This serves the same bundle Cloudflare will serve, including `_headers`
processing, on a local port.
