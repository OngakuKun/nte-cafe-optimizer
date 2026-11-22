(() => {
  const THEME_KEY = 'nte-cafe-theme';
  const IMPORTED_THEMES_KEY = 'nte-cafe-imported-themes';
  const BUILTIN_THEMES = [
    { id: 'latte', name: 'Catppuccin Latte' },
    { id: 'frappe', name: 'Catppuccin Frappe' },
    { id: 'macchiato', name: 'Catppuccin Macchiato' },
    { id: 'mocha', name: 'Catppuccin Mocha' },
  ];
  const REQUIRED_VARS = ['rosewater','flamingo','pink','mauve','red','maroon','peach','yellow','green','teal','sky','sapphire','blue','lavender','text','subtext1','subtext0','overlay2','overlay1','overlay0','surface2','surface1','surface0','base','mantle','crust'];

  let importedThemes = loadImportedThemes();

  function loadImportedThemes() {
    try {
      const raw = localStorage.getItem(IMPORTED_THEMES_KEY);
      if (!raw) return [];
      return normalizeImportedThemes(JSON.parse(raw)).themes;
    } catch (_) { return []; }
  }
  function saveImportedThemes() {
    try { localStorage.setItem(IMPORTED_THEMES_KEY, JSON.stringify({ themes: importedThemes })); } catch (_) {}
  }
  function allThemes() { return [...BUILTIN_THEMES, ...importedThemes]; }
  function themeIds() { return allThemes().map(t => t.id); }
  function preferredTheme() {
    try { return window.matchMedia('(prefers-color-scheme: light)').matches ? 'latte' : 'macchiato'; }
    catch (_) { return 'macchiato'; }
  }
  function normalizeImportedThemes(input) {
    const root = input && typeof input === 'object' ? input : null;
    if (!root || !Array.isArray(root.themes)) throw new Error('Invalid theme.json: expected a top-level "themes" array.');
    const seen = new Set(BUILTIN_THEMES.map(t => t.id));
    const themes = [];
    for (let i = 0; i < root.themes.length; i++) {
      const t = root.themes[i];
      if (!t || typeof t !== 'object') throw new Error(`Theme ${i + 1} is not an object.`);
      const id = String(t.id || '').trim();
      const name = String(t.name || '').trim();
      if (!/^[a-z0-9-]+$/.test(id)) throw new Error(`Theme ${i + 1} has invalid id "${t.id}". Use lowercase letters, numbers, dashes.`);
      if (!name) throw new Error(`Theme "${id}" is missing a name.`);
      if (seen.has(id)) throw new Error(`Duplicate or reserved theme id "${id}".`);
      const vars = t.vars && typeof t.vars === 'object' ? t.vars : null;
      if (!vars) throw new Error(`Theme "${id}" is missing a vars object.`);
      const cleanVars = {};
      for (const k of REQUIRED_VARS) {
        const v = vars[k];
        if (typeof v !== 'string' || !v.trim()) throw new Error(`Theme "${id}" is missing var "${k}".`);
        cleanVars[k] = v.trim();
      }
      seen.add(id);
      themes.push({ id, name, vars: cleanVars });
    }
    if (themes.length === 0) throw new Error('No themes found in theme.json.');
    return { themes };
  }
  function applyImportedThemeCss() {
    let el = document.getElementById('imported-theme-css');
    if (!el) { el = document.createElement('style'); el.id = 'imported-theme-css'; document.head.appendChild(el); }
    el.textContent = importedThemes.map(t => {
      const body = REQUIRED_VARS.map(k => `--ctp-${k}: ${t.vars[k]};`).join(' ');
      return `[data-theme="${t.id}"] { ${body} }`;
    }).join('\n');
  }
  function rebuildThemeSelect() {
    const select = document.getElementById('themeSelect');
    if (!select) return;
    const current = document.documentElement.dataset.theme;
    select.innerHTML = '';
    for (const t of BUILTIN_THEMES) {
      const opt = document.createElement('option'); opt.value = t.id; opt.textContent = t.name; select.appendChild(opt);
    }
    if (importedThemes.length) {
      const group = document.createElement('optgroup'); group.label = 'Imported';
      for (const t of importedThemes) { const opt = document.createElement('option'); opt.value = t.id; opt.textContent = t.name; group.appendChild(opt); }
      select.appendChild(group);
    }
    if (themeIds().includes(current)) select.value = current;
  }
  function applyTheme(theme, persist) {
    const ids = themeIds();
    const next = ids.includes(theme) ? theme : preferredTheme();
    document.documentElement.dataset.theme = next;
    const select = document.getElementById('themeSelect');
    if (select && ids.includes(next)) select.value = next;
    if (persist) { try { localStorage.setItem(THEME_KEY, next); } catch (_) {} }
  }
  async function importThemeFile(file) {
    const err = document.getElementById('error');
    if (err) err.textContent = '';
    const parsed = JSON.parse(await file.text());
    const { themes } = normalizeImportedThemes(parsed);
    const map = new Map(importedThemes.map(t => [t.id, t]));
    for (const t of themes) map.set(t.id, t);
    importedThemes = Array.from(map.values());
    saveImportedThemes(); applyImportedThemeCss(); rebuildThemeSelect(); applyTheme(themes[themes.length - 1].id, true);
  }
  function initSettingsPanel() {
    const btn = document.getElementById('settingsBtn');
    const panel = document.getElementById('settingsPanel');
    if (!btn || !panel) return;
    const close = () => { panel.classList.remove('open'); btn.setAttribute('aria-expanded', 'false'); };
    const open = () => { panel.classList.add('open'); btn.setAttribute('aria-expanded', 'true'); };
    btn.addEventListener('click', (e) => { e.stopPropagation(); panel.classList.contains('open') ? close() : open(); });
    panel.addEventListener('click', (e) => e.stopPropagation());
    document.addEventListener('click', close);
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
  }

  function initNav() {
    const nav = document.querySelector('header nav');
    const headerRight = document.querySelector('.header-right');
    if (!nav || !headerRight) return;
    let btn = document.getElementById('menuBtn');
    let overlay = document.getElementById('navOverlay');
    if (!btn) {
      btn = document.createElement('button');
      btn.id = 'menuBtn'; btn.className = 'icon-btn menu-btn'; btn.type = 'button';
      btn.setAttribute('aria-label', 'Menu'); btn.setAttribute('aria-expanded', 'false'); btn.setAttribute('aria-controls', nav.id || 'siteNav');
      btn.innerHTML = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18v2H3zm0 5h18v2H3zm0 5h18v2H3z"/></svg>';
      headerRight.insertBefore(btn, headerRight.firstChild);
    }
    if (!nav.id) nav.id = 'siteNav';
    if (!overlay) {
      overlay = document.createElement('div');
      overlay.id = 'navOverlay'; overlay.className = 'nav-overlay';
      document.body.appendChild(overlay);
    }
    const close = () => { nav.classList.remove('open'); overlay.classList.remove('open'); btn.setAttribute('aria-expanded', 'false'); };
    const open = () => { nav.classList.add('open'); overlay.classList.add('open'); btn.setAttribute('aria-expanded', 'true'); };
    btn.addEventListener('click', (e) => { e.stopPropagation(); nav.classList.contains('open') ? close() : open(); });
    overlay.addEventListener('click', close);
    nav.addEventListener('click', (e) => { if (e.target.closest('a')) close(); });
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    window.addEventListener('resize', () => { if (window.innerWidth > 900) close(); });
  }

  function initTheme() {
    applyImportedThemeCss(); rebuildThemeSelect();
    let saved = ''; try { saved = localStorage.getItem(THEME_KEY) || ''; } catch (_) {}
    applyTheme(themeIds().includes(saved) ? saved : preferredTheme(), false);
    const select = document.getElementById('themeSelect');
    if (select) select.addEventListener('change', (e) => applyTheme(e.target.value, true));
    const btn = document.getElementById('importThemeBtn');
    const file = document.getElementById('importThemeFile');
    if (btn && file) {
      btn.addEventListener('click', () => file.click());
      file.addEventListener('change', async (e) => {
        const f = e.target.files && e.target.files[0]; e.target.value = ''; if (!f) return;
        try { await importThemeFile(f); }
        catch (ex) { const err = document.getElementById('error'); if (err) err.textContent = 'Theme import failed: ' + (ex && ex.message ? ex.message : String(ex)); }
      });
    }
    initSettingsPanel();
    initNav();
  }
  window.NteTheme = { initTheme, applyTheme };
})();
