#!/usr/bin/env node
import fs from 'node:fs/promises';
import process from 'node:process';
import path from 'node:path';

const repoRoot = process.cwd();
const failures = [];
const warnings = [];

function fail(message) {
  failures.push(message);
}

function warn(message) {
  warnings.push(message);
}

function ensure(condition, message) {
  if (!condition) {
    fail(message);
  }
}

function hostFromUrl(candidate) {
  try {
    return new URL(candidate).hostname;
  } catch {
    return null;
  }
}

async function readText(relativePath) {
  return fs.readFile(path.join(repoRoot, relativePath), 'utf8');
}

async function main() {
  const appJs = await readText('pwa/app.js');
  const indexHtml = await readText('pwa/index.html');

  const allowedHosts = new Set(['api.github.com', 'api.castalia.institute']);
  const allowedStorageKeys = new Set(['nocturne-pwa-session', 'nocturne-offline-snapshot']);

  const stringFetchCalls = [];
  const stringUrlCalls = [];
  const stringLocalStorageSet = [];
  const stringLocalStorageGet = [];
  const stringLocalStorageRemove = [];

  const fetchMatches = [...appJs.matchAll(/fetch\(\s*(['"])(.*?)\1/g)];
  for (const match of fetchMatches) {
    stringFetchCalls.push(match[2]);
  }

  const urlMatches = [...appJs.matchAll(/https?:\/\/[^\s'"]+/g)];
  for (const match of urlMatches) {
    stringUrlCalls.push(match[0]);
  }

  const setMatches = [...appJs.matchAll(/localStorage\.setItem\(\s*(['"])(.*?)\1/g)];
  for (const match of setMatches) {
    stringLocalStorageSet.push(match[2]);
  }
  const getMatches = [...appJs.matchAll(/localStorage\.getItem\(\s*(['"])(.*?)\1/g)];
  for (const match of getMatches) {
    stringLocalStorageGet.push(match[2]);
  }
  const removeMatches = [...appJs.matchAll(/localStorage\.removeItem\(\s*(['"])(.*?)\1/g)];
  for (const match of removeMatches) {
    stringLocalStorageRemove.push(match[2]);
  }

  for (const target of stringFetchCalls) {
    const host = hostFromUrl(target);
    if (!host) {
      fail(`Non-URL fetch target detected in app.js: ${target}`);
      continue;
    }
    ensure(allowedHosts.has(host), `Unapproved network host in app.js fetch call: ${host}`);
  }

  const writeKeys = new Set([...stringLocalStorageSet, ...stringLocalStorageGet, ...stringLocalStorageRemove]);
  for (const key of writeKeys) {
    ensure(allowedStorageKeys.has(key), `Unapproved localStorage key in app.js: ${key}`);
  }

  for (const target of stringUrlCalls) {
    const normalized = target.split('${')[0];
    const host = hostFromUrl(normalized);
    if (!host) {
      fail(`Unparsable URL in app.js: ${target}`);
      continue;
    }
    ensure(allowedHosts.has(host), `Unapproved network URL literal in app.js: ${target}`);
  }

  ensure(appJs.includes('requirePermission('), 'Permission policy gate function missing');
  ensure(appJs.includes('permissionState'), 'Permission state tracking missing');
  ensure(indexHtml.includes('id=\"permission-status\"'), 'Permission status UI element missing in index.html');
  ensure(!/console\.log\(/.test(appJs), 'Console logging detected in app.js (prefer controlled logLine).');
  ensure(!/alert\(/.test(appJs), 'Uncontrolled alert() usage detected in app.js.');

  if (stringLocalStorageSet.length === 0) {
    warn('No localStorage.setItem calls found in app.js');
  }
  for (const warning of warnings) {
    console.log(`warning: ${warning}`);
  }
  for (const message of failures) {
    console.error(`error: ${message}`);
  }

  if (failures.length > 0) {
    console.error(`privacy audit failed: ${failures.length} error(s), ${warnings.length} warning(s)`);
    process.exit(1);
  }

  console.log(`privacy audit passed with ${warnings.length} warning(s)`);
}

main().catch((error) => {
  console.error(`privacy audit crashed: ${error.message}`);
  process.exit(1);
});
