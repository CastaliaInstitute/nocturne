#!/usr/bin/env node
import fs from 'node:fs/promises';
import path from 'node:path';

const [, , pluginName = 'my_adapter', stateName = 'MyState'] = process.argv;
const root = new URL('..', import.meta.url).pathname;
const templatePath = path.join(root, 'templates/adapter/nocturne_adapter_template.c');
const outPath = path.join(root, `${pluginName}.c`);

const source = await fs.readFile(templatePath, 'utf8');
const rendered = source
  .replaceAll('${PLUGIN_NAME}', pluginName)
  .replaceAll('${PLUGIN_STATE_NAME}', stateName);

await fs.writeFile(outPath, rendered, 'utf8');
console.log(`wrote ${outPath}`);
