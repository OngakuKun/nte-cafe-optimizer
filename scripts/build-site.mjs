#!/usr/bin/env node
/*
 * Generate docs/ (GitHub Pages) from the current app UI.
 *
 *   node scripts/build-site.mjs
 *
 * Sources, in order of precedence:
 *   1. app/src/ui/            the app itself — the real source of truth
 *   2. site/overrides/        whole files that replace an app UI file
 *   3. site/replacements.json literal string replacements in a named file
 *   4. site/webui.js          static stand-in for the WebUI/backend bindings
 *
 * Afterwards all absolute app paths are rewritten for a project Pages URL and
 * the CSS/JS links get a cache-busting token from the current commit.
 */
import { cpSync, copyFileSync, existsSync, mkdirSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const uiDir = join(root, 'app', 'src', 'ui');
const siteDir = join(root, 'site');
/* Default: docs/ for a local preview. CI sets SITE_OUT to a temp directory
 * before publishing the very same files to the gh-pages branch. */
const docsDir = process.env.SITE_OUT ? resolve(process.env.SITE_OUT) : join(root, 'docs');

function siteVersion() {
  if (process.env.GITHUB_SHA) return process.env.GITHUB_SHA.slice(0, 7);
  try {
    return execFileSync('git', ['rev-parse', '--short', 'HEAD'], { cwd: root, encoding: 'utf8' }).trim();
  } catch (_) {
    return 'dev';
  }
}

const version = siteVersion();

rmSync(docsDir, { recursive: true, force: true });
mkdirSync(docsDir, { recursive: true });
cpSync(uiDir, docsDir, { recursive: true, force: true });
copyFileSync(join(root, 'data', 'gamedata.json'), join(docsDir, 'gamedata.json'));
copyFileSync(join(siteDir, 'webui.js'), join(docsDir, 'webui.js'));

/* Whole-file overrides: site/overrides/index.html replaces app/src/ui/index.html. */
const overridesDir = join(siteDir, 'overrides');
if (existsSync(overridesDir)) {
  for (const name of readdirSync(overridesDir)) {
    if (name.startsWith('.')) continue;
    const source = join(overridesDir, name);
    const target = join(docsDir, name);
    if (statSync(source).isDirectory()) cpSync(source, target, { recursive: true, force: true });
    else copyFileSync(source, target);
  }
}

/* Small differences: literal replacements, applied before path rewriting. */
const replacementsFile = join(siteDir, 'replacements.json');
if (existsSync(replacementsFile)) {
  const replacements = JSON.parse(readFileSync(replacementsFile, 'utf8'));
  for (const [name, pairs] of Object.entries(replacements)) {
    const file = join(docsDir, name);
    if (!existsSync(file)) throw new Error(`site/replacements.json targets missing file: ${name}`);
    let text = readFileSync(file, 'utf8');
    for (const [from, to] of pairs) {
      if (!text.includes(from)) throw new Error(`replacement not found in ${name}: ${from.slice(0, 80)}`);
      text = text.replaceAll(from, to);
    }
    writeFileSync(file, text);
  }
}

/* App -> project Pages paths, plus a version token so Brave cannot cache stale assets. */
for (const name of readdirSync(docsDir)) {
  if (!name.endsWith('.html')) continue;
  const file = join(docsDir, name);
  let text = readFileSync(file, 'utf8');
  text = text.replace(/(src|href)="(webui\.js|shared\.css|theme\.js)(\?v=[^"]*)?"/g, '$1="$2"');
  text = text.replace('src="/webui.js"', `src="webui.js?v=${version}"`);
  text = text.replace('href="/ui/shared.css"', `href="shared.css?v=${version}"`);
  text = text.replace('src="/ui/theme.js"', `src="theme.js?v=${version}"`);
  text = text.replaceAll('src="/ui/assets/', 'src="assets/');
  text = text.replaceAll('href="/"', 'href="index.html"');
  for (const page of ['characters', 'dishes', 'cafes', 'status']) {
    text = text.replaceAll(`href="/${page}"`, `href="${page}.html"`);
  }
  text = text.replaceAll('href="/${k}"', 'href="${k}.html"');
  writeFileSync(file, text);
}

writeFileSync(join(docsDir, '.nojekyll'), '');
console.log(`Generated docs/ from app/src/ui/ (v=${version}): ${readdirSync(docsDir).length} entries`);
