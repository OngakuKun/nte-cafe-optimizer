# Project site sources

`docs/` is generated — don't edit it by hand. Run `node scripts/build-site.mjs`
for a local preview, or push to `dev`: the workflow in
`.github/workflows/site.yml` regenerates `docs/` and publishes the same files
to the root of the dedicated `gh-pages` branch, which is what GitHub Pages
serves (Settings → Pages → **gh-pages / (root)**). Set `SITE_OUT=/some/dir` to
generate somewhere else; CI uses that for the gh-pages checkout.

Order of precedence:

1. `app/src/ui/` — the app itself, the source of truth for the whole UI.
2. `site/overrides/` — drop a file here to replace the same-named app UI file
   completely (e.g. `site/overrides/index.html`). Subdirectories work too.
3. `site/replacements.json` — small literal text differences, applied before
   path rewriting. Example:

   ```json
   {
     "index.html": [
       ["Project site", "Website"],
       ["href=\"/cafes\"", "href=\"cafes.html\""]
     ]
   }
   ```

   Every `from` string must exist in the file or the build fails loudly.
4. `site/webui.js` — static stand-in for the WebUI bindings from
   `webui_bind()` in `app/src/main.c`. Keep its binding names in sync when the
   app adds or changes bindings.

Afterwards the script rewrites absolute app paths (`/webui.js`, `/ui/shared.css`,
`/characters`, …) for the `/nte-cafe-optimizer/` project URL and stamps CSS/JS
links with `?v=<commit>` so browsers pick up changes immediately.
