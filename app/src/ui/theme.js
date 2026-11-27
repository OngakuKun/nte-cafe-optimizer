(() => {
  const THEME_KEY = 'nte-cafe-theme';
  const IMPORTED_THEMES_KEY = 'nte-cafe-imported-themes';
  const LANG_KEY = 'nte-cafe-lang';
  const LANGS = [
    { id: 'en', name: 'English' },
    { id: 'de', name: 'Deutsch' },
  ];
  const GAMEDATA_URL = 'https://raw.githubusercontent.com/OngakuKun/nte-cafe-optimizer/dev/data/gamedata.json';
  const GAMEDATA_CHECK_KEY = 'nte-cafe-gamedata-check';
  let GAME_I18N = {};
  /* Canonical names from the NTE wiki page "The Café by Origen" (same source as the images). */
  const NAME_MAP = {
    'Ice Mocha': 'Iced Mocha',
    'Tomato & Fried Egg': 'Tomato & Fried Eggs Croissant',
    'Matacha Lava Mousse': 'Matcha Lava Mousse',
    'Red Bean & Butter Crossiant': 'Red Bean & Butter Croissant',
    'Pork Cutlet': 'Pork Cutlet Sandwich',
    'Ham & Cheese': 'Ham & Cheese Croissant',
    'Dafodil': 'Daffodill',
  };
  /* German display names; keys are canonical wiki names. */
  const DE_NAMES = {
    'Coconut Latte': 'Kokos-Latte',
    'Iced Mocha': 'Eis-Mokka',
    'Apple Pie': 'Apfelkuchen',
    'Tomato & Fried Eggs Croissant': 'Croissant mit Tomaten und Spiegelei',
    'Tuna Sandwich': 'Thunfisch-Sandwich',
    'Amber Glamor': 'Goldglanz-Traum',
    'Matcha Lava Mousse': 'Matcha-Lave-Mousse',
    'Snowy Latte': 'Snowy Latte',
    'Thick-Cut Fresh Beef Sandwich': 'Frisches dickes Rindfleisch-Sandwich',
    'Red Bean & Butter Croissant': 'Croissant mit roten Bohnen und Butter',
    'Strawberry Box Cake': 'Erdbeer-Kastenkuchen',
    'Orange Hot Coffee': 'Heißer Orangen-Kaffee',
    'Pork Cutlet Sandwich': 'Schweineschnitzel-Sandwich',
    'Coconut Macchiato': 'Kokos-Macchiato',
    'Matcha Latte with Milk Cap': 'Milchschaum-Matcha-Latte',
    /* 'Tiramisu' is identical in both languages — no mapping needed. */
    'Ham & Cheese Croissant': 'Schinken-Käse-Croissant',
    'Caramel & Cocoa Mille-Feuille': 'Mille-Feuille mit Karamell und Kakao',
  };
  /* Trend option labels. Keys are the raw backend values — never translate the value itself. */
  const TREND_DE = {
    'None': 'Ohne Trend',
    'Main Dishes': 'Hauptgericht',
    'Desserts': 'Dessert',
    'Beverages': 'Getränke',
    'Fruit': 'Obst',
    'Coffee Beans': 'Kaffeebohnen',
    'Coconut': 'Kokosnuss',
    'Cocoa Powder': 'Kakaopulver',
    'Flour': 'Mehl',
    'Apple': 'Apfel',
    'Eggs': 'Eier',
    'Lettuce': 'Salat',
    'Fish': 'Fisch',
    'Orange': 'Orange',
    'Milk': 'Milch',
    'Matcha Powder': 'Matchapulver',
    'Whipping Cream': 'Schlagsahne',
    'Beef': 'Rindfleisch',
    'Butter': 'Butter',
    'Red Beans': 'Rote Bohnen',
    'Strawberry': 'Erdbeere',
    'Onion': 'Zwiebel',
    'Pork': 'Schweinefleisch',
    'Salt': 'Salz',
    'Cookies': 'Kekse',
    'Cheese': 'Käse',
    'Ham': 'Schinken',
    'Sugar': 'Zucker',
    'Skibbles': 'Skibbles',
    'Tomato': 'Tomate',
  };
  const TREND_EN = {
    'None': 'No Trend',
    'Main Dishes': 'Main Dish',
    'Desserts': 'Dessert',
    'Beverages': 'Beverage',
  };
  /* Backend skill-tag identifiers -> canonical trend keys (used on the Status page). */
  const SKILL_TAG_ALIAS = {
    'none': 'None',
    'any': 'None',
    'beverages': 'Beverages',
    'desserts': 'Desserts',
    'main_dishes': 'Main Dishes',
  };
  const I18N = {
    en: {
      'title.optimizer':'NTE Cafe - Optimizer','title.characters':'NTE Cafe - Characters','title.dishes':'NTE Cafe - Dishes','title.cafes':'NTE Cafe - Cafes','title.status':'NTE Cafe - Status',
      'nav.optimizer':'Optimizer','nav.characters':'Characters','nav.dishes':'Dishes','nav.cafes':'Cafes','nav.status':'Status','nav.menu':'Menu',
      'sub.optimizer':'Roster optimization workspace','sub.characters':'Character ownership and levels','sub.dishes':'Dish ownership and levels','sub.cafes':'Cafe ownership and popularity bonuses','sub.status':'Backend status and loaded data',
      'status.loading':'Loading...','status.connected':'Connected','status.error':'Error',
      'settings.title':'Settings','settings.group.appearance':'Appearance','settings.group.data':'Data','settings.config':'Config','settings.exportConfig':'Export Config','settings.importConfig':'Import Config','settings.reset':'Reset to Defaults','settings.language':'Language','settings.theme':'Theme','settings.import':'Import Theme','settings.reload':'Reload',
      'common.saving':'Saving...','common.saved':'Saved','common.saveFailed':'Save failed',
      'common.noBackend':'WebUI backend bindings unavailable. Open through nte-cafe-app server.',
      'common.noBackendOpt':'Optimization binding unavailable. Restart nte-cafe-app.',
      'common.optimizationFailed':'Optimization failed','common.failed':'Failed: ',
      'export.exporting':'Exporting…','export.importing':'Importing…','export.done':'Exported {a} characters, {d} dishes.','export.failed':'Export failed: ',
      'import.done':'Imported {a} characters, {d} dishes.','import.failed':'Import failed: ',
      'import.invalid':'Not an NTE Cafe config (format/version mismatch).','import.missing':'Config is missing the characters or dishes array.',
      'theme.importFailed':'Theme import failed: ',
      'overview.loading':'Loading overview...','overview.characters':'Characters:','overview.dishes':'Dishes:','overview.charCombos':'Char combos:','overview.dishCombos':'Dish combos:','overview.total':'Total:','overview.owned':'owned','overview.slots':'slots',
      'cafe.owned':'Cafes Owned','cafe.ownedDesc':'Set how many cafes are used for optimization.','cafe.badge':'Cafes','cafe.count':'Count','cafe.edit':'Edit cafes →','cafe.one':'1 cafe','cafe.many':'{n} cafes',
      'trend.title':'Trend','trend.current':'Current trend','trend.bonus':'Trend bonus:','trend.none':'None','trend.group.dishTypes':'Dish Types','trend.group.categories':'Categories','trend.group.ingredients':'Ingredients',
      'run.optimizer':'Run Optimizer',
      'decor.title':"Hotori's Cat Decor",'decor.desc':'Apply the Hotori cat decor bonus in optimization.','decor.alt':'Exclusive Customer Service decor','decor.bonus':'+1% base traffic','decor.enabled':'Enabled','decor.disabled':'Disabled',
      'result.title':'Result','result.empty':'Run optimizer to see results.','result.calculating':'Calculating...','result.traffic':'Traffic','result.income':'Income','result.priceBuff':'Price Buff','result.popularityBonus':'Popularity Bonus','result.hours':'in {h}h','result.trend':'Trend:','result.inTrend':'In trend','result.noChars':'No characters selected.','result.noDishes':'No dishes selected.','result.perHour':'/h',
      'default.title':'Welcome to NTE Cafe Optimizer','default.desc':'Your setup is still at defaults. Configure your owned characters, dishes, and cafe settings so the optimizer can calculate meaningful results.','default.configureChars':'Configure Characters','default.configureDishes':'Configure Dishes',
      'setup.title':'Setup incomplete','setup.descBoth':'Own at least 3 characters at L1 or higher and 2 dishes so the optimizer can calculate meaningful results.','setup.descChars':'Own at least 3 characters at L1 or higher so the optimizer can calculate meaningful results.','setup.descDishes':'Own at least 2 dishes so the optimizer can calculate meaningful results.',
      'summary.characters':'Character active: {a}/{n} ({b})','summary.dishes':'Dishes active: {a}/{n} ({b})',
      'bulk.max':'All to Max Level','bulk.min':'All to Min Level','bulk.owned':'All to Owned','bulk.unowned':'All to Unowned',
      'dialog.confirm':'Confirm','dialog.cancel':'Cancel','dialog.reset':'Reset','dialog.resetTitle':'Reset to Defaults',
      'reset.defaults':'Reset the whole setup to defaults? All characters and dishes become "Not Owned", cafés go to 1, trend to None, Hotori decor off and popularity to 0. All settings will be lost.',
      'card.owned':'Owned','card.notOwned':'Not Owned','card.level':'Level','card.lv':'Lv {n}','card.toggleOwned':'Toggle {name} owned','card.levelFor':'Level for {name}',
      'skill.price':'Price','skill.traffic':'Traffic','skill.req':'req',
      'dish.prices':'L{a} {p1} · L{b} {p2}',
      'type.Beverages':'Beverages','type.Desserts':'Desserts','type.Main Dishes':'Main Dishes',
      'cafe.popularityLabel':'Total multiplier for all cafes','cafe.cafesOwned':'Cafes Owned','cafe.level':'Level {n} · Popularity {p}','cafe.active':'Active','cafe.locked':'Locked','cafe.noBonus':'No bonus','cafe.currentBonus':'Current bonus:',
      'cafe.name.1':'Bluebeard Road Location','cafe.name.2':'Fiscus Avenue Location','cafe.name.3':'Davidia Avenue Location','cafe.name.4':'Moomin Street Location','cafe.name.5':'Hankaku Street Location','cafe.name':'Cafe {n}',
      'status.gameVersion':'Game Version','status.webuiVersion':'WebUI Version','status.dishesLoaded':'Dishes Loaded','status.charsLoaded':'Characters Loaded',
      'status.section.dishes':'Dishes','status.section.characters':'Characters',
      'status.col.num':'#','status.col.name':'Name','status.col.type':'Type','status.col.priceL1':'Price L1','status.col.priceL2':'Price L2','status.col.ingredients':'Ingredients','status.col.skills':'Skills','status.col.skillDetails':'Skill Details'
    },
    de: {
      'title.optimizer':'NTE Cafe - Optimierer','title.characters':'NTE Cafe - Charaktere','title.dishes':'NTE Cafe - Gerichte','title.cafes':'NTE Cafe - Cafes','title.status':'NTE Cafe - Status',
      'nav.optimizer':'Optimierer','nav.characters':'Charaktere','nav.dishes':'Gerichte','nav.cafes':'Cafes','nav.status':'Status','nav.menu':'Menü',
      'sub.optimizer':'Arbeitsbereich für Besetzungsoptimierung','sub.characters':'Charakterbesitz und Stufen','sub.dishes':'Gerichtbesitz und Stufen','sub.cafes':'Café-Beliebtheit und Boni','sub.status':'Backend-Status und geladene Daten',
      'status.loading':'Lädt...','status.connected':'Verbunden','status.error':'Fehler',
      'settings.title':'Einstellungen','settings.group.appearance':'Darstellung','settings.group.data':'Daten','settings.config':'Config','settings.exportConfig':'Config exportieren','settings.importConfig':'Config importieren','settings.reset':'Auf Standard zurücksetzen','settings.language':'Sprache','settings.theme':'Design','settings.import':'Design importieren','settings.reload':'Neu laden',
      'common.saving':'Speichern...','common.saved':'Gespeichert','common.saveFailed':'Speichern fehlgeschlagen',
      'common.noBackend':'WebUI-Bindings nicht verfügbar. Über den nte-cafe-app-Server öffnen.',
      'common.noBackendOpt':'Optimizer-Binding nicht verfügbar. nte-cafe-app neu starten.',
      'common.optimizationFailed':'Optimierung fehlgeschlagen','common.failed':'Fehlgeschlagen: ',
      'export.exporting':'Exportiere…','export.importing':'Importiere…','export.done':'{a} Charaktere und {d} Gerichte exportiert.','export.failed':'Export fehlgeschlagen: ',
      'import.done':'{a} Charaktere und {d} Gerichte importiert.','import.failed':'Import fehlgeschlagen: ',
      'import.invalid':'Keine NTE-Cafe-Config (Format/Version unbekannt).','import.missing':'Config enthält keine Character- oder Dishes-Liste.',
      'theme.importFailed':'Design-Import fehlgeschlagen: ',
      'overview.loading':'Lädt Übersicht...','overview.characters':'Charaktere:','overview.dishes':'Gerichte:','overview.charCombos':'Char-Kombis:','overview.dishCombos':'Gericht-Kombis:','overview.total':'Gesamt:','overview.owned':'im Besitz','overview.slots':'Slots',
      'cafe.owned':'Cafes im Besitz','cafe.ownedDesc':'Lege fest, wie viele Cafes optimiert werden.','cafe.badge':'Cafes','cafe.count':'Anzahl','cafe.edit':'Cafes bearbeiten →','cafe.one':'1 Café','cafe.many':'{n} Cafes',
      'trend.title':'Trend','trend.current':'Aktueller Trend','trend.bonus':'Trend-Bonus:','trend.none':'Keiner','trend.group.dishTypes':'Gerichttypen','trend.group.categories':'Kategorien','trend.group.ingredients':'Zutaten',
      'run.optimizer':'Optimierer starten',
      'decor.title':"Hotori's Katzendekor",'decor.desc':'Hotori-Katzendeko-Bonus bei der Optimierung berücksichtigen.','decor.alt':'Exklusive Kundenservice-Deko','decor.bonus':'+1% Aufkommen','decor.enabled':'Aktiviert','decor.disabled':'Deaktiviert',
      'result.title':'Ergebnis','result.empty':'Optimierer starten, um Ergebnisse zu sehen.','result.calculating':'Berechne...','result.traffic':'Aufkommen','result.income':'Einkommen','result.priceBuff':'Preis-Buff','result.popularityBonus':'Beliebtheitsbonus','result.hours':'in {h} Std.','result.trend':'Trend:','result.inTrend':'Im Trend','result.noChars':'Keine Charaktere ausgewählt.','result.noDishes':'Keine Gerichte ausgewählt.','result.perHour':'/Std.',
      'default.title':'Willkommen beim NTE Cafe Optimizer','default.desc':'Deine Einstellungen sind noch standardmäßig. Konfiguriere deine Charaktere, Gerichte und Cafes, damit der Optimierer sinnvolle Ergebnisse berechnen kann.','default.configureChars':'Charaktere konfigurieren','default.configureDishes':'Gerichte konfigurieren',
      'setup.title':'Einstellungen unvollständig','setup.descBoth':'Besitze mindestens 3 Charaktere auf mindestens L1 und 2 Gerichte, damit der Optimierer sinnvolle Ergebnisse berechnen kann.','setup.descChars':'Besitze mindestens 3 Charaktere auf mindestens L1, damit der Optimierer sinnvolle Ergebnisse berechnen kann.','setup.descDishes':'Besitze mindestens 2 Gerichte, damit der Optimierer sinnvolle Ergebnisse berechnen kann.',
      'summary.characters':'Charaktere aktiv: {a}/{n} ({b})','summary.dishes':'Gerichte aktiv: {a}/{n} ({b})',
      'bulk.max':'Alle auf Max-Stufe','bulk.min':'Alle auf Min-Stufe','bulk.owned':'Alle besitzen','bulk.unowned':'Alle nicht besitzen',
      'dialog.confirm':'Bestätigen','dialog.cancel':'Abbrechen','dialog.reset':'Zurücksetzen','dialog.resetTitle':'Auf Standard zurücksetzen',
      'reset.defaults':'Die gesamte Konfiguration auf Standard zurücksetzen? Alle Charaktere und Gerichte sind dann „Nicht in Besitz", die Cafés auf 1, der Trend auf Keiner, das Hotori-Dekor aus und die Beliebtheit auf 0. Alle Einstellungen gehen verloren.',
      'card.owned':'In Besitz','card.notOwned':'Nicht in Besitz','card.level':'Stufe','card.lv':'Lv {n}','card.toggleOwned':'Besitz von {name} umschalten','card.levelFor':'Stufe für {name}',
      'skill.price':'Preis','skill.traffic':'Aufkommen','skill.req':'Benötigt',
      'dish.prices':'L{a} {p1} · L{b} {p2}',
      'type.Beverages':'Getränke','type.Desserts':'Desserts','type.Main Dishes':'Hauptgerichte',
      'cafe.popularityLabel':'Gesamtmultiplikator für alle Läden','cafe.cafesOwned':'Cafes im Besitz','cafe.level':'Stufe {n} · Beliebtheit {p}','cafe.active':'Aktiv','cafe.locked':'Gesperrt','cafe.noBonus':'Kein Bonus','cafe.currentBonus':'Aktueller Bonus:',
      'cafe.name.1':'Standort Blaubart-Straße','cafe.name.2':'Standort Fiskus-Allee','cafe.name.3':'Standort Davidia-Allee Nord','cafe.name.4':'Standort Mumin-Straße','cafe.name.5':'Standort Hankaku-Straße','cafe.name':'Café {n}',
      'status.gameVersion':'Spielversion','status.webuiVersion':'WebUI-Version','status.dishesLoaded':'Gerichte geladen','status.charsLoaded':'Charaktere geladen',
      'status.section.dishes':'Gerichte','status.section.characters':'Charaktere',
      'status.col.num':'#','status.col.name':'Name','status.col.type':'Typ','status.col.priceL1':'Preis L1','status.col.priceL2':'Preis L2','status.col.ingredients':'Zutaten','status.col.skills':'Skills','status.col.skillDetails':'Skill-Details'
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
    return nav.startsWith('de') ? 'de' : 'en';
  }
  function t(key, vars) {
    const lang = document.documentElement.lang || 'en';
    let s = (I18N[lang] && I18N[lang][key]) ?? (I18N.en && I18N.en[key]) ?? key;
    if (vars) for (const [k, v] of Object.entries(vars)) s = s.replaceAll('{' + k + '}', String(v));
    return s;
  }
  /* Canonical wiki name — stable key for image lookups, independent of UI language. */
  function canonicalName(name) { return NAME_MAP[name] || name; }
  /* Localised display name. The JSON dataset is the source of truth; the maps
   * below stay as an offline fallback. */
  function displayName(name) {
    const canonical = canonicalName(name);
    if ((document.documentElement.lang || 'en') !== 'de') return canonical;
    return GAME_I18N['dish.' + canonical] || DE_NAMES[canonical] || canonical;
  }
  function trendName(name) {
    const lang = document.documentElement.lang || 'en';
    const key = SKILL_TAG_ALIAS[name] || name;
    const fromJson = GAME_I18N['trend.' + key];
    if (fromJson) return fromJson;
    const dict = lang === 'de' ? TREND_DE : TREND_EN;
    return dict[key] || name;
  }
  async function loadGameI18n() {
    if (typeof window.getI18n !== 'function') return;
    try {
      const lang = document.documentElement.lang || 'en';
      const parsed = JSON.parse(await window.getI18n(lang));
      if (parsed && typeof parsed === 'object') GAME_I18N = parsed;
    } catch (e) {
      console.warn('Game translations unavailable; using built-in map.', e);
    }
  }
  async function checkGameDataUpdate() {
    try {
      for (let i = 0; i < 50 && typeof getAppInfo !== 'function'; i++)
        await new Promise(r => setTimeout(r, 100));
      if (typeof getAppInfo !== 'function' || typeof updateGamedata !== 'function') return;

      let last = null;
      try { last = JSON.parse(localStorage.getItem(GAMEDATA_CHECK_KEY) || 'null'); } catch (_) {}
      const now = Date.now();
      if (last && now - (last.at || 0) < 6 * 60 * 60 * 1000) return;

      const info = JSON.parse(await getAppInfo());
      const res = await fetch(GAMEDATA_URL, { cache: 'no-store' });
      if (!res.ok) return;
      const text = await res.text();
      const remote = JSON.parse(text);
      try { localStorage.setItem(GAMEDATA_CHECK_KEY, JSON.stringify({ at: now })); } catch (_) {}
      if (!remote || remote.format !== 'nte-cafe-gamedata') return;
      if (remote.gameVersion === info.gameVersion && remote.contentHash === info.contentHash) return;

      const result = JSON.parse(await updateGamedata(text));
      if (!result.ok) return;
      try { localStorage.setItem(GAMEDATA_CHECK_KEY, JSON.stringify({ at: now, hash: result.contentHash })); } catch (_) {}
      location.reload();
    } catch (e) {
      console.warn('Game data update check skipped.', e);
    }
  }
  function applyLanguage(lang, persist) {
    const next = LANGS.some(l => l.id === lang) ? lang : 'en';
    document.documentElement.lang = next;
    if (persist) { try { localStorage.setItem(LANG_KEY, next); } catch (_) {} }
    const sel = document.getElementById('langSelect');
    if (sel) sel.value = next;
    document.querySelectorAll('[data-i18n]').forEach(el => {
      const v = t(el.dataset.i18n);
      if (v != null) el.textContent = v;
    });
    document.querySelectorAll('[data-i18n-aria]').forEach(el => {
      const v = t(el.dataset.i18nAria);
      if (v != null) el.setAttribute('aria-label', v);
    });
    document.querySelectorAll('[data-i18n-alt]').forEach(el => {
      const v = t(el.dataset.i18nAlt);
      if (v != null) el.setAttribute('alt', v);
    });
    document.dispatchEvent(new CustomEvent('nte-langchange', { detail: { lang: next } }));
  }
  async function waitForBindings(names, timeoutMs = 8000) {
    const start = Date.now();
    while (Date.now() - start < timeoutMs) {
      if (names.every(n => typeof window[n] === 'function')) return true;
      await new Promise(r => setTimeout(r, 100));
    }
    return names.every(n => typeof window[n] === 'function');
  }
  function configStatus(msg, kind, withReload) {
    const el = document.getElementById('configStatus');
    if (!el) return;
    el.textContent = msg || '';
    el.className = 'settings-hint' + (kind ? ' ' + kind : '');
    if (withReload) {
      const b = document.createElement('button');
      b.type = 'button'; b.className = 'config-reload';
      b.textContent = t('settings.reload') || 'Reload';
      b.addEventListener('click', () => location.reload());
      el.appendChild(b);
    }
  }
  function checkResult(raw) {
    const obj = JSON.parse(raw);
    if (obj && obj.ok === false) throw new Error(obj.error || t('common.saveFailed'));
    return obj;
  }
  /* Export/import use the same ID-keyed format as the CLI's config.json,
   * produced and validated by the shared C config module. */
  async function exportConfig() {
    if (!await waitForBindings(['exportConfig'])) throw new Error(t('common.noBackend'));
    const cfg = JSON.parse(await window.exportConfig());
    if (!cfg || cfg.ok === false) throw new Error((cfg && cfg.error) || t('export.failed'));
    const url = URL.createObjectURL(new Blob([JSON.stringify(cfg, null, 2)], { type: 'application/json' }));
    const a = document.createElement('a');
    a.href = url;
    a.download = 'config.json';
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
    configStatus(t('export.done', {
      a: Object.keys(cfg.characters || {}).length,
      d: Object.keys(cfg.dishes || {}).length
    }), 'ok');
  }
  async function importConfigFile(file) {
    const text = await file.text();
    let cfg;
    try { cfg = JSON.parse(text); } catch (_) { throw new Error(t('import.invalid')); }
    if (!cfg || cfg.format !== 'nte-cafe-config')
      throw new Error(t('import.invalid'));
    if (!cfg.characters || !cfg.dishes)
      throw new Error(t('import.missing'));
    if (!await waitForBindings(['importConfig'])) throw new Error(t('common.noBackend'));
    const res = checkResult(await window.importConfig(text));
    if (res.warning) console.warn('Config import:', res.warning);
    configStatus(t('import.done', {
      a: Object.keys(cfg.characters).length,
      d: Object.keys(cfg.dishes).length
    }), 'ok', true);
  }
  function initSettingsPanel() {
    const eb = document.getElementById('exportConfigBtn');
    const ib = document.getElementById('importConfigBtn');
    const rb = document.getElementById('resetDefaultsBtn');
    const file = document.getElementById('importConfigFile');
    if (eb) eb.addEventListener('click', async () => {
      configStatus(t('export.exporting'));
      try { await exportConfig(); }
      catch (e) { configStatus(t('export.failed') + (e && e.message ? e.message : String(e)), 'error'); }
    });
    if (ib && file) {
      ib.addEventListener('click', () => file.click());
      file.addEventListener('change', async (e) => {
        const f = e.target.files && e.target.files[0]; e.target.value = ''; if (!f) return;
        configStatus(t('export.importing'));
        try { await importConfigFile(f); }
        catch (ex) { configStatus(t('import.failed') + (ex && ex.message ? ex.message : String(ex)), 'error'); }
      });
    }
    if (rb) rb.addEventListener('click', () => resetDefaultsDialog());
  }

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

    const resetScroll = () => { menu.scrollTop = 0; menu.scrollTo?.(0, 0); };
    const close = () => {
      if (!wrap.classList.contains('open')) return;
      wrap.classList.remove('open');
      btn.setAttribute('aria-expanded', 'false');
      resetScroll();
    };
    wrap._closeCselect = close;
    new MutationObserver(() => {
      menu.scrollTop = 0;
      if (wrap.classList.contains('open')) requestAnimationFrame(() => { menu.scrollTop = 0; });
    }).observe(wrap, { attributes: true, attributeFilter: ['class'] });
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
      document.querySelectorAll('.cselect.open').forEach(c => c._closeCselect ? c._closeCselect() : c.classList.remove('open'));
      if (!open) {
        wrap.classList.add('open');
        btn.setAttribute('aria-expanded', 'true');
        resetScroll();
        requestAnimationFrame(() => { resetScroll(); setTimeout(resetScroll, 0); });
      }
    });
    document.addEventListener('click', (e) => { if (!wrap.contains(e.target)) close(); });
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    select.addEventListener('change', sync);
    new MutationObserver(sync).observe(select, { childList: true, subtree: true, attributes: true });
    const desc = Object.getOwnPropertyDescriptor(HTMLSelectElement.prototype, 'value');
    if (desc && desc.set && desc.get) {
      Object.defineProperty(select, 'value', {
        get() { return desc.get.call(this); },
        set(v) { desc.set.call(this, v); sync(); },
        configurable: true,
      });
    }
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
      btn.setAttribute('aria-label', t('nav.menu')); btn.setAttribute('data-i18n-aria', 'nav.menu'); btn.setAttribute('aria-expanded', 'false'); btn.setAttribute('aria-controls', nav.id || 'siteNav');
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
        <div class="settings-title" data-i18n="settings.title">Settings</div>
        <div class="settings-group">
          <div class="settings-group-title" data-i18n="settings.group.appearance">Appearance</div>
          <div class="settings-row">
            <label for="langSelect" data-i18n="settings.language">Language</label>
            <select id="langSelect" class="theme-select" data-i18n-aria="settings.language"></select>
            <label for="themeSelect" data-i18n="settings.theme">Theme</label>
            <select id="themeSelect" class="theme-select" aria-label="Color theme"></select>
            <button id="importThemeBtn" class="theme-btn" type="button" data-i18n="settings.import">Import Theme</button>
            <input id="importThemeFile" type="file" accept=".json,application/json" hidden>
          </div>
        </div>
        <div class="settings-group">
          <div class="settings-group-title" id="configGroupTitle" data-i18n="settings.group.data">Data</div>
          <div class="settings-config-row">
            <button id="exportConfigBtn" class="theme-btn" type="button" data-i18n="settings.exportConfig">Export Config</button>
            <button id="importConfigBtn" class="theme-btn" type="button" data-i18n="settings.importConfig">Import Config</button>
          </div>
          <div class="settings-config-row">
            <button id="resetDefaultsBtn" class="theme-btn warning" type="button" data-i18n="settings.reset">Reset to Defaults</button>
          </div>
          <input id="importConfigFile" type="file" accept=".json,application/json" hidden>
          <div id="configStatus" class="settings-hint"></div>
        </div>`;
    }

    let settingsBtn = document.getElementById('settingsBtn');
    let settingsPanel = document.getElementById('settingsPanel');
    if (!settingsBtn) {
      const wrap = document.createElement('div');
      wrap.className = 'settings-wrap';
      wrap.innerHTML = `<button id="settingsBtn" class="icon-btn settings-btn" type="button" aria-label="Settings" data-i18n-aria="settings.title" aria-expanded="false"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19.14 12.94a7.43 7.43 0 0 0 .05-.94 7.43 7.43 0 0 0-.05-.94l2.03-1.58a.5.5 0 0 0 .12-.64l-1.92-3.32a.5.5 0 0 0-.6-.22l-2.39.96a7.28 7.28 0 0 0-1.63-.94l-.36-2.54A.5.5 0 0 0 13.9 2h-3.8a.5.5 0 0 0-.49.42l-.36 2.54c-.58.23-1.12.54-1.63.94l-2.39-.96a.5.5 0 0 0-.6.22L2.71 8.48a.5.5 0 0 0 .12.64l2.03 1.58a7.43 7.43 0 0 0-.05.94 7.43 7.43 0 0 0 .05.94l-2.03 1.58a.5.5 0 0 0-.12.64l1.92 3.32a.5.5 0 0 0 .6.22l2.39-.96c.5.4 1.05.71 1.63.94l.36 2.54a.5.5 0 0 0 .49.42h3.8a.5.5 0 0 0 .49-.42l.36-2.54c.58-.23 1.12-.54 1.63-.94l2.39.96a.5.5 0 0 0 .6-.22l1.92-3.32a.5.5 0 0 0-.12-.64l-2.03-1.58ZM12 15.5A3.5 3.5 0 1 1 12 8a3.5 3.5 0 0 1 0 7.5Z"/></svg></button><div id="settingsPanel" class="settings-panel" role="dialog" aria-label="Settings" data-i18n-aria="settings.title"></div>`;
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

  /* Shared shell for the in-app popup, used in the browser and desktop WebView. */
  function createDialogShell() {
    const previous = document.getElementById('nteConfirmDialog');
    if (previous) previous.remove();
    const backdrop = document.createElement('div');
    backdrop.id = 'nteConfirmDialog';
    backdrop.className = 'nte-dialog-backdrop';
    const box = document.createElement('div');
    box.className = 'nte-dialog';
    box.setAttribute('role', 'alertdialog');
    box.setAttribute('aria-modal', 'true');
    const title = document.createElement('div');
    title.className = 'nte-dialog-title';
    const text = document.createElement('p');
    text.className = 'nte-dialog-message';
    const actions = document.createElement('div');
    actions.className = 'modal-actions nte-dialog-actions';
    box.append(title, text, actions);
    backdrop.appendChild(box);
    document.body.appendChild(backdrop);
    let onKey = () => {};
    const key = (fn) => {
      document.removeEventListener('keydown', onKey);
      onKey = fn;
      document.addEventListener('keydown', onKey);
    };
    const close = () => { document.removeEventListener('keydown', onKey); backdrop.remove(); };
    const button = (label, cls, onClick) => {
      const b = document.createElement('button');
      b.type = 'button'; b.className = cls; b.textContent = label;
      b.addEventListener('click', onClick);
      return b;
    };
    const set = (titleText, message, buttons) => {
      title.textContent = titleText;
      text.textContent = message;
      actions.textContent = '';
      buttons.forEach(b => actions.appendChild(b));
      if (buttons.length) buttons[buttons.length - 1].focus();
    };
    return { backdrop, key, close, button, set };
  }

  /* Central in-app confirmation dialog. Replaces window.confirm so the same
   * styled popup is used in the browser and the desktop WebView. Resolves to
   * true only on the confirm button; Escape, backdrop and Cancel resolve false. */
  function confirmDialog(message, options = {}) {
    return new Promise((resolve) => {
      const d = createDialogShell();
      let settled = false;
      const finish = (value) => {
        if (settled) return;
        settled = true;
        d.close();
        resolve(value);
      };
      d.set(options.title || t('dialog.confirm'), message, [
        d.button(t('dialog.cancel'), 'modal-btn', () => finish(false)),
        d.button(options.confirmLabel || t('dialog.confirm'),
                 options.danger === false ? 'modal-btn primary' : 'modal-btn danger',
                 () => finish(true))
      ]);
      d.key((e) => {
        if (e.key === 'Escape') { e.preventDefault(); finish(false); }
        else if (e.key === 'Enter') { e.preventDefault(); finish(true); }
      });
      d.backdrop.addEventListener('click', (e) => { if (e.target === d.backdrop) finish(false); });
    });
  }

  /* Reset keeps the confirmation popup, then reloads the page itself so the
   * fresh state is shown without an extra click. */
  function resetDefaultsDialog() {
    const d = createDialogShell();
    const cancel = d.button(t('dialog.cancel'), 'modal-btn', () => d.close());
    const reset = d.button(t('dialog.reset'), 'modal-btn danger', async () => {
      cancel.disabled = true; reset.disabled = true;
      try {
        if (!await waitForBindings(['resetDefaults'])) throw new Error(t('common.noBackend'));
        checkResult(await window.resetDefaults());
        location.reload();
      } catch (ex) {
        d.set(t('dialog.resetTitle'), t('common.saveFailed') + (ex && ex.message ? ' ' + ex.message : String(ex)), [
          d.button(t('dialog.cancel'), 'modal-btn', () => d.close())
        ]);
      }
    });
    d.set(t('dialog.resetTitle'), t('reset.defaults'), [cancel, reset]);
    d.key((e) => {
      if (e.key === 'Escape') { e.preventDefault(); d.close(); }
      else if (e.key === 'Enter') { e.preventDefault(); reset.click(); }
    });
    d.backdrop.addEventListener('click', (e) => { if (e.target === d.backdrop) d.close(); });
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
        catch (ex) { const err = document.getElementById('error'); if (err) err.textContent = t('theme.importFailed') + (ex && ex.message ? ex.message : String(ex)); }
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
    loadGameI18n();
    checkGameDataUpdate();
  }
  window.NteTheme = { initTheme, applyTheme, applyLanguage, enhanceSelects, confirmDialog, exportConfig, importConfigFile, t, displayName, canonicalName, trendName, loadGameI18n, checkGameDataUpdate };
})();
