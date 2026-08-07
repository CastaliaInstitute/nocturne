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
- Generative Web Audio soundscape playback (iOS-safe)
- Offline snapshot dump and service-worker bootstrap

## Soundscape playback and iPhone notes

Playback is fully generative (Web Audio oscillators plus filtered noise through a
soft limiter), so no audio files or network access are required.

iOS Safari imposes rules the app handles explicitly:

- Audio can only start from a user gesture, so playback begins with the
  "Play Soundscape" tap and the `AudioContext` is created/resumed inside that
  handler (with a one-sample unlock buffer for older iOS).
- Locking the phone or switching apps interrupts the context; the app resumes it
  automatically when the page becomes visible again.
- The hardware ringer/silent switch mutes Web Audio unless the audio session is
  promoted to `playback`; the app sets `navigator.audioSession.type = 'playback'`
  where supported (iOS 16.4+). On older iOS versions, flip the silent switch off
  if you hear nothing.
- Media Session metadata is registered so lock-screen controls show the
  soundscape and can stop it.

If an installed copy of the app seems stale (e.g. no playback panel), the old
service-worker cache is the likely cause; the cache version is bumped on each
release so a reload after the new worker activates picks up current assets.

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
- Static hosting target should serve:
  - `pwa/index.html`
  - `pwa/manifest.webmanifest`
  - `pwa/service-worker.js`
  - `pwa/app.js`
  - `pwa/styles.css`

## Missing assets

- `icon-192.png` and `icon-512.png` placeholders are required before production.
