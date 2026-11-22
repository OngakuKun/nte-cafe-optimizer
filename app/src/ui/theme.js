(() => {
  const THEME_KEY = 'nte-cafe-theme';
  const IMPORTED_THEMES_KEY = 'nte-cafe-imported-themes';
  const LANG_KEY = 'nte-cafe-lang';
  const LANGS = [
    { id: 'en', name: 'English' },
    { id: 'de', name: 'Deutsch' },
    { id: 'ja', name: '日本語' },
    { id: 'zh-CN', name: '简体中文' },
  ];
  const I18N = {
    de: {
      'nav.optimizer':'Optimierer','nav.characters':'Charaktere','nav.dishes':'Gerichte','nav.status':'Status',
      'settings.title':'Einstellungen','settings.theme':'Design','settings.import':'Design importieren','settings.language':'Sprache'
    },
    ja: {
      'nav.optimizer':'最適化','nav.characters':'キャラクター','nav.dishes':'料理','nav.status':'ステータス',
      'settings.title':'設定','settings.theme':'テーマ','settings.import':'テーマをインポート','settings.language':'言語',
      'sub.optimizer':'編成最適化ワークスペース','sub.characters':'スタッフの所有状況とレベル','sub.dishes':'メニューの所有状況とレベル','sub.status':'バックエンドとライブラリ情報'
    },
    'zh-CN': {
      'nav.optimizer':'优化器','nav.characters':'角色','nav.dishes':'菜品','nav.status':'状态',
      'settings.title':'设置','settings.theme':'主题','settings.import':'导入主题','settings.language':'语言',
      'sub.optimizer':'阵容优化工作区','sub.characters':'员工拥有情况与等级','sub.dishes':'菜单拥有情况与等级','sub.status':'后端与库信息'
    }
  };
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
  function currentLang() {
    try { const s = localStorage.getItem(LANG_KEY); if (LANGS.some(l => l.id === s)) return s; } catch (_) {}
    const nav = (navigator.language || 'en').toLowerCase();
    if (nav.startsWith('de')) return 'de';
    if (nav.startsWith('ja')) return 'ja';
    if (nav.startsWith('zh')) return 'zh-CN';
    return 'en';
  }
  function t(key) {
    const lang = document.documentElement.lang || 'en';
    return (I18N[lang] && I18N[lang][key]) || (I18N.en && I18N.en[key]) || null;
  }
  function applyLanguage(lang, persist) {
    const next = LANGS.some(l => l.id === lang) ? lang : 'en';
    document.documentElement.lang = next;
    if (persist) { try { localStorage.setItem(LANG_KEY, next); } catch (_) {} }
    const sel = document.getElementById('langSelect');
    if (sel) sel.value = next;
    const dict = I18N[next] || {};
    const map = { '/':'nav.optimizer','/characters':'nav.characters','/dishes':'nav.dishes','/status':'nav.status' };
    document.querySelectorAll('nav a.nav-link').forEach(a => {
      const k = map[a.getAttribute('href')]; if (k && dict[k]) a.textContent = dict[k];
    });
    const st = document.querySelector('#navSettings .settings-title'); if (st) st.textContent = dict['settings.title'] || 'Settings';
    const tl = document.querySelector('label[for="themeSelect"]'); if (tl) tl.textContent = dict['settings.theme'] || 'Theme';
    const ib = document.getElementById('importThemeBtn'); if (ib) ib.textContent = dict['settings.import'] || 'Import Theme';
    const ll = document.querySelector('label[for="langSelect"]'); if (ll) ll.textContent = dict['settings.language'] || 'Language';
    const sub = document.querySelector('.sub');
    if (sub) {
      if (!sub.dataset.defaultText) sub.dataset.defaultText = sub.textContent;
      const path = location.pathname.replace(/\/+$/, '') || '/';
      const sk = map[path] ? map[path].replace('nav.', 'sub.') : null;
      sub.textContent = (sk && dict[sk]) ? dict[sk] : sub.dataset.defaultText;
    }
  }
  function initSettingsPanel() {}

  function enhanceSelect(select) {
    if (!select || select.dataset.enhanced) return;
    select.dataset.enhanced = '1';
    select.classList.add('enhanced-select');
    const wrap = document.createElement('div');
    wrap.className = 'cselect' + (select.classList.contains('compact-select') ? ' compact' : '') + (select.classList.contains('dropup-select') ? ' dropup' : '');
    const btn = document.createElement('button');
    btn.type = 'button'; btn.className = 'cselect-btn'; btn.setAttribute('aria-haspopup', 'listbox');
    const menu = document.createElement('div');
    menu.className = 'cselect-menu'; menu.setAttribute('role', 'listbox');
    wrap.appendChild(btn); wrap.appendChild(menu);
    select.insertAdjacentElement('afterend', wrap);

    const close = () => { menu.scrollTop = 0; wrap.classList.remove('open'); btn.setAttribute('aria-expanded', 'false'); requestAnimationFrame(() => { menu.scrollTop = 0; }); };
    const sync = () => {
      menu.innerHTML = '';
      const addOption = (opt) => {
        const b = document.createElement('button');
        b.type = 'button'; b.className = 'cselect-option' + (opt.selected ? ' selected' : '');
        b.textContent = opt.textContent; b.dataset.value = opt.value; b.setAttribute('role', 'option');
        b.addEventListener('click', (e) => {
          e.stopPropagation();
          select.value = opt.value;
          select.dispatchEvent(new Event('change', { bubbles: true }));
          sync(); close();
        });
        menu.appendChild(b);
      };
      Array.from(select.children).forEach(child => {
        if (child.tagName === 'OPTGROUP') {
          const label = document.createElement('div');
          label.className = 'cselect-group';
          label.textContent = child.label;
          menu.appendChild(label);
          Array.from(child.children).forEach(o => { if (o.tagName === 'OPTION') addOption(o); });
        } else if (child.tagName === 'OPTION') {
          addOption(child);
        }
      });
      const sel = select.options[select.selectedIndex] || select.options[0];
      btn.textContent = sel ? sel.textContent : '';
      btn.setAttribute('aria-expanded', wrap.classList.contains('open') ? 'true' : 'false');
    };
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const open = wrap.classList.contains('open');
      document.querySelectorAll('.cselect.open').forEach(c => { c.classList.remove('open'); c.querySelector('.cselect-btn')?.setAttribute('aria-expanded','false'); const m = c.querySelector('.cselect-menu'); if (m) m.scrollTop = 0; });
      if (!open) { wrap.classList.add('open'); btn.setAttribute('aria-expanded','true'); menu.scrollTop = 0; requestAnimationFrame(() => { menu.scrollTop = 0; }); }
    });
    document.addEventListener('click', (e) => { if (!wrap.contains(e.target)) close(); });
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    select.addEventListener('change', sync);
    new MutationObserver(sync).observe(select, { childList: true, subtree: true, attributes: true });
    sync();
  }
  function enhanceSelects(root = document) {
    root.querySelectorAll('select.theme-select, select.input').forEach(enhanceSelect);
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
      headerRight.appendChild(btn);
    }
    if (!nav.id) nav.id = 'siteNav';
    if (!overlay) { overlay = document.createElement('div'); overlay.id = 'navOverlay'; overlay.className = 'nav-overlay'; document.body.appendChild(overlay); }

    let settings = document.getElementById('navSettings');
    if (!settings) {
      settings = document.createElement('div');
      settings.id = 'navSettings'; settings.className = 'nav-settings';
      settings.innerHTML = `
        <div class="settings-title">Settings</div>
        <div class="settings-row">
          <label for="langSelect">Language</label>
          <select id="langSelect" class="theme-select" aria-label="Language"></select>
          <label for="themeSelect">Theme</label>
          <select id="themeSelect" class="theme-select" aria-label="Color theme"></select>
          <button id="importThemeBtn" class="theme-btn" type="button">Import Theme</button>
          <input id="importThemeFile" type="file" accept=".json,application/json" hidden>
        </div>`;
    }

    let settingsBtn = document.getElementById('settingsBtn');
    let settingsPanel = document.getElementById('settingsPanel');
    if (!settingsBtn) {
      const wrap = document.createElement('div');
      wrap.className = 'settings-wrap';
      wrap.innerHTML = `<button id="settingsBtn" class="icon-btn settings-btn" type="button" aria-label="Settings" aria-expanded="false"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19.14 12.94a7.43 7.43 0 0 0 .05-.94 7.43 7.43 0 0 0-.05-.94l2.03-1.58a.5.5 0 0 0 .12-.64l-1.92-3.32a.5.5 0 0 0-.6-.22l-2.39.96a7.28 7.28 0 0 0-1.63-.94l-.36-2.54A.5.5 0 0 0 13.9 2h-3.8a.5.5 0 0 0-.49.42l-.36 2.54c-.58.23-1.12.54-1.63.94l-2.39-.96a.5.5 0 0 0-.6.22L2.71 8.48a.5.5 0 0 0 .12.64l2.03 1.58a7.43 7.43 0 0 0-.05.94 7.43 7.43 0 0 0 .05.94l-2.03 1.58a.5.5 0 0 0-.12.64l1.92 3.32a.5.5 0 0 0 .6.22l2.39-.96c.5.4 1.05.71 1.63.94l.36 2.54a.5.5 0 0 0 .49.42h3.8a.5.5 0 0 0 .49-.42l.36-2.54c.58-.23 1.12-.54 1.63-.94l2.39.96a.5.5 0 0 0 .6-.22l1.92-3.32a.5.5 0 0 0-.12-.64l-2.03-1.58ZM12 15.5A3.5 3.5 0 1 1 12 8a3.5 3.5 0 0 1 0 7.5Z"/></svg></button><div id="settingsPanel" class="settings-panel" role="dialog" aria-label="Settings"></div>`;
      headerRight.appendChild(wrap);
      settingsBtn = wrap.querySelector('#settingsBtn');
      settingsPanel = wrap.querySelector('#settingsPanel');
    }

    const updateHeaderH = () => { const h = document.querySelector('header'); if (h) document.documentElement.style.setProperty('--header-h', h.offsetHeight + 'px'); };
    updateHeaderH();
    const placeSettings = () => {
      updateHeaderH();
      if (window.innerWidth <= 900) {
        if (settings.parentElement !== nav) nav.appendChild(settings);
        settingsPanel.classList.remove('open'); settingsBtn.setAttribute('aria-expanded', 'false'); overlay.classList.remove('open');
      } else {
        if (settings.parentElement !== settingsPanel) settingsPanel.appendChild(settings);
      }
    };
    placeSettings();

    const closeNav = () => { nav.classList.remove('open'); overlay.classList.remove('open'); btn.setAttribute('aria-expanded', 'false'); };
    const openNav = () => { nav.classList.add('open'); overlay.classList.add('open'); btn.setAttribute('aria-expanded', 'true'); };
    const closeSettings = () => { settingsPanel.classList.remove('open'); settingsBtn.setAttribute('aria-expanded', 'false'); if (window.innerWidth > 900) overlay.classList.remove('open'); };
    const openSettings = () => { settingsPanel.classList.add('open'); settingsBtn.setAttribute('aria-expanded', 'true'); overlay.classList.add('open'); closeNav(); };

    btn.addEventListener('click', (e) => { e.stopPropagation(); nav.classList.contains('open') ? closeNav() : openNav(); });
    settingsBtn.addEventListener('click', (e) => { e.stopPropagation(); settingsPanel.classList.contains('open') ? closeSettings() : openSettings(); });
    overlay.addEventListener('click', () => { closeNav(); closeSettings(); });
    nav.addEventListener('click', (e) => { if (e.target.closest('a')) closeNav(); });
    settingsPanel.addEventListener('click', (e) => e.stopPropagation());
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') { closeNav(); closeSettings(); } });
    window.addEventListener('resize', () => { updateHeaderH(); if (window.innerWidth > 900) closeNav(); if (window.innerWidth <= 900) closeSettings(); placeSettings(); });
  }

  function initTheme() {
    initNav();
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
    const langSelect = document.getElementById('langSelect');
    if (langSelect) {
      langSelect.innerHTML = LANGS.map(l => `<option value="${l.id}">${l.name}</option>`).join('');
      langSelect.addEventListener('change', (e) => applyLanguage(e.target.value, true));
    }
    applyLanguage(currentLang(), false);
    enhanceSelects(document);
    initSettingsPanel();
  }
  window.NteTheme = { initTheme, applyTheme, applyLanguage, enhanceSelects };
})();
