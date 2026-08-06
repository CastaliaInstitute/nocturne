#!/usr/bin/env node
import fs from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const repoRoot = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const failures = [];
const warnings = [];

function fail(message) {
  failures.push(message);
}

function warn(message) {
  warnings.push(message);
}

function ensure(cond, message) {
  if (!cond) {
    fail(message);
  }
}

async function fileExists(relativePath) {
  try {
    const stat = await fs.stat(path.join(repoRoot, relativePath));
    return stat.isFile();
  } catch {
    return false;
  }
}

async function run() {
  const requiredFiles = [
    'pwa/index.html',
    'pwa/app.js',
    'pwa/styles.css',
    'pwa/manifest.webmanifest',
    'pwa/service-worker.js',
  ];

  for (const file of requiredFiles) {
    ensure(await fileExists(file), `Missing required file: ${file}`);
  }

  let indexHtml = '';
  let manifestText = '';
  let appJs = '';
  try {
    indexHtml = await fs.readFile(path.join(repoRoot, 'pwa/index.html'), 'utf8');
    manifestText = await fs.readFile(path.join(repoRoot, 'pwa/manifest.webmanifest'), 'utf8');
    appJs = await fs.readFile(path.join(repoRoot, 'pwa/app.js'), 'utf8');
  } catch (error) {
    fail(`Failed to read PWA files: ${error.message}`);
  }

  if (!indexHtml.includes('manifest.webmanifest')) {
    fail('index.html does not reference manifest.webmanifest');
  }
  if (!appJs.includes('serviceWorker.register') && !appJs.includes('./service-worker.js')) {
    fail('PWA has no service-worker registration path');
  }

  let manifest;
  try {
    manifest = JSON.parse(manifestText);
  } catch (error) {
    fail(`Malformed manifest JSON: ${error.message}`);
    manifest = null;
  }

  if (manifest) {
    ensure(!!manifest.name, 'Manifest missing required field: name');
    ensure(!!manifest.short_name, 'Manifest missing required field: short_name');
    ensure(!!manifest.start_url, 'Manifest missing required field: start_url');
    ensure(!!manifest.display, 'Manifest missing required field: display');
    ensure(Array.isArray(manifest.icons), 'Manifest missing required field: icons');
    const iconWarnings = [
      ...(manifest.icons || []).map((icon) => icon && icon.src),
    ];
    for (const src of iconWarnings) {
      if (!src) continue;
      const iconPath = path.join('pwa', src);
      if (!(await fileExists(iconPath))) {
        warn(`Manifest references missing icon asset: ${iconPath}`);
      }
    }
  }

  const hostedUrl = (process.env.NOCTURNE_PWA_URL || '').trim();
  if (hostedUrl) {
    try {
      const res = await fetch(hostedUrl);
      ensure(res.ok, `Smoke GET ${hostedUrl} returned ${res.status}`);
    } catch (error) {
      fail(`Unable to reach hosted URL ${hostedUrl}: ${error.message}`);
    }

    if (manifest) {
      try {
        const manifestRes = await fetch(new URL('manifest.webmanifest', hostedUrl.endsWith('/') ? hostedUrl : `${hostedUrl}/`));
        ensure(manifestRes.ok, `Smoke GET ${hostedUrl}/manifest.webmanifest returned ${manifestRes.status}`);
      } catch (error) {
        fail(`Unable to fetch hosted manifest from ${hostedUrl}: ${error.message}`);
      }
    }
  } else {
    warn('NOCTURNE_PWA_URL is not set; skipping live endpoint checks');
  }

  for (const warning of warnings) {
    console.log(`warning: ${warning}`);
  }
  for (const failure of failures) {
    console.error(`error: ${failure}`);
  }

  if (failures.length > 0) {
    console.error(`pwa smoke checks failed: ${failures.length} error(s), ${warnings.length} warning(s)`);
    process.exit(1);
  }
  console.log(`pwa smoke checks passed with ${warnings.length} warning(s)`);
}

run().catch((error) => {
  console.error(`fatal: ${error.message}`);
  process.exit(1);
});
