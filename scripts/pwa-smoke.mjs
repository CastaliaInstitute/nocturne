#!/usr/bin/env node
// Smoke test for the Nocturne PWA static bundle.
// Verifies the deployable pwa/ directory is internally consistent before a
// Cloudflare deploy: manifest parses, every referenced asset exists, and the
// service worker precache list points at real files.
import { readFileSync, existsSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const pwaDir = join(dirname(fileURLToPath(import.meta.url)), '..', 'pwa');
const failures = [];

const require_ = (cond, msg) => {
  if (!cond) failures.push(msg);
};

// Core files Cloudflare must serve
for (const f of ['index.html', 'app.js', 'styles.css', 'service-worker.js', 'manifest.webmanifest', '_headers']) {
  require_(existsSync(join(pwaDir, f)), `missing required file: pwa/${f}`);
}

// Manifest parses and its icons exist
try {
  const manifest = JSON.parse(readFileSync(join(pwaDir, 'manifest.webmanifest'), 'utf8'));
  require_(typeof manifest.name === 'string' && manifest.name.length > 0, 'manifest: missing name');
  require_(Array.isArray(manifest.icons) && manifest.icons.length > 0, 'manifest: missing icons');
  for (const icon of manifest.icons ?? []) {
    require_(existsSync(join(pwaDir, icon.src)), `manifest icon not found: pwa/${icon.src}`);
  }
} catch (err) {
  failures.push(`manifest.webmanifest does not parse: ${err.message}`);
}

// Service worker precache entries exist
const sw = readFileSync(join(pwaDir, 'service-worker.js'), 'utf8');
for (const m of sw.matchAll(/'\.\/([^']+)'/g)) {
  require_(existsSync(join(pwaDir, m[1])), `service worker caches missing file: pwa/${m[1]}`);
}

// index.html references exist
const html = readFileSync(join(pwaDir, 'index.html'), 'utf8');
for (const m of html.matchAll(/(?:href|src)="([^"#][^":]*)"/g)) {
  require_(existsSync(join(pwaDir, m[1])), `index.html references missing file: pwa/${m[1]}`);
}

if (failures.length > 0) {
  console.error('PWA smoke test FAILED:');
  for (const f of failures) console.error(`  - ${f}`);
  process.exit(1);
}
console.log('PWA smoke test passed.');
