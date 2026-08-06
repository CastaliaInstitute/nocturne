#!/usr/bin/env node
import fs from 'node:fs/promises';
import path from 'node:path';

const ROOT_DIR = new URL('.', import.meta.url);

function usage() {
  return `Usage:
  node scripts/nocturne-cli.mjs validate-pack-metadata <file>
  node scripts/nocturne-cli.mjs print-fused-intent [file]
  node scripts/nocturne-cli.mjs deterministic-dryrun [mode] [iterations]
`;
}

function hashString(value) {
  let h = 2166136261 >>> 0;
  for (let i = 0; i < value.length; i += 1) {
    h ^= value.charCodeAt(i);
    h = Math.imul(h, 16777619);
  }
  return (h >>> 0).toString(16).padStart(8, '0');
}

function deterministicSeed(seedBase, i) {
  const step = ((seedBase * 1103515245 + 12345) ^ (i * 2654435761)) >>> 0;
  return step / 0xffffffff;
}

async function readJson(filePath) {
  const raw = await fs.readFile(filePath, 'utf8');
  return JSON.parse(raw);
}

async function validatePackMetadata(file) {
  const payload = await readJson(file);
  const required = ['name', 'version'];
  const missing = required.filter((key) => !(key in payload));
  if (missing.length > 0) {
    throw new Error(`pack metadata missing required key(s): ${missing.join(', ')}`);
  }
  if (typeof payload.version !== 'string') {
    throw new Error('pack metadata version must be a string');
  }
  return {
    ok: true,
    name: payload.name,
    version: payload.version,
  };
}

function normalizeIntentFrame(entry) {
  if (!entry || !entry.intent) {
    return null;
  }
  return {
    grounding: entry.intent.grounding,
    brightness: entry.intent.brightness,
    warmth: entry.intent.warmth,
    density: entry.intent.density,
    motion: entry.intent.motion,
    spaciousness: entry.intent.spaciousness,
    silence: entry.intent.silence,
    event_activity: entry.intent.event_activity,
    harmonic_tension: entry.intent.harmonic_tension,
    novelty: entry.intent.novelty,
  };
}

async function printFusedIntent(filePath) {
  const payload = await readJson(filePath);
  const timeline = Array.isArray(payload.frames)
    ? payload.frames
    : Array.isArray(payload)
      ? payload
      : [payload];

  const lines = timeline
    .map((frame, idx) => ({
      index: idx,
      timestamp: frame.timestamp ?? frame.last_update_ms ?? 'n/a',
      intent: normalizeIntentFrame(frame),
    }))
    .filter((item) => item.intent)
    .map((item) => `${item.index}\t${item.timestamp}\tgrounding=${item.intent.grounding.toFixed(3)} brightness=${item.intent.brightness.toFixed(3)} density=${item.intent.density.toFixed(3)} novelty=${item.intent.novelty.toFixed(3)}`);

  if (lines.length === 0) {
    throw new Error('no intent frames found in input payload');
  }
  console.log(lines.join('\n'));
}

function deterministicDryRun(mode = 'sleep', iterations = 5) {
  const safeMode = Number.isFinite(iterations) && iterations > 0 ? Math.floor(iterations) : 5;
  const modeBias = mode === 'focus' ? 0.25 : mode === 'ritual' ? 0.1 : 0.15;
  let state = 123456789;
  const out = [];
  for (let i = 0; i < safeMode; i += 1) {
    const r = deterministicSeed(state, i + 1);
    const metric = modeBias + (r * 0.8);
    out.push({
      index: i,
      mode,
      loudness: Number((metric * 0.95).toFixed(4)),
      density: Number((0.4 + modeBias + i * 0.01).toFixed(4)),
      event_activity: Number((0.2 + modeBias + r * 0.3).toFixed(4)),
      novelty: Number((0.05 + modeBias * 0.5 + r * 0.1).toFixed(4)),
    });
    state = (state * 1664525 + 1013904223) >>> 0;
  }
  return out;
}

async function main() {
  const [, , command, arg1, arg2] = process.argv;
  try {
    if (command === 'validate-pack-metadata') {
      if (!arg1) {
        console.error('file path is required');
        process.exitCode = 1;
        return;
      }
      const target = path.resolve(path.dirname(new URL(import.meta.url).pathname), '..', arg1);
      const result = await validatePackMetadata(target);
      console.log(JSON.stringify(result, null, 2));
      return;
    }

    if (command === 'print-fused-intent') {
      const file = arg1
        ? path.resolve(path.dirname(new URL(import.meta.url).pathname), '..', arg1)
        : path.resolve(path.dirname(new URL(import.meta.url).pathname), '..', 'core/tests/fixtures/intent-frame.json');
      await printFusedIntent(file);
      return;
    }

    if (command === 'deterministic-dryrun') {
      const out = deterministicDryRun(arg1 || 'sleep', Number.parseInt(arg2 || '5', 10));
      console.log(JSON.stringify(out, null, 2));
      return;
    }

    console.error(usage());
    process.exitCode = 1;
  } catch (error) {
    console.error(`error: ${error.message}`);
    process.exitCode = 1;
  }
}

await main();
