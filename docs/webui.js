/*
 * Static stand-in for the WebUI bridge on GitHub Pages.
 *
 * The site is a verbatim copy of the app UI; the real app binds these names to
 * C functions in app/src/main.c. Here they are implemented in JavaScript with
 * the same game data and state format, so every page works without a server.
 */
(() => {
  'use strict';

  const STATE_KEY = 'nte-site-state-v1';
  const MAX_CAFES = 5;
  const TIERS = [
    [[50, 0.005], [100, 0.007], [150, 0.010]],
    [[100, 0.003], [200, 0.006], [300, 0.009], [400, 0.012], [500, 0.015]],
    [[100, 0.003], [200, 0.006], [300, 0.009], [400, 0.012], [500, 0.015]],
    [[150, 0.005], [300, 0.010], [450, 0.015], [600, 0.020], [750, 0.025]],
    [[150, 0.005], [300, 0.010], [450, 0.015], [600, 0.020], [750, 0.025]]
  ];

  let data = null;
  let state = null;
  const ready = fetch('gamedata.json')
    .then((r) => { if (!r.ok) throw new Error('gamedata.json unavailable'); return r.json(); })
    .then((d) => { data = d; state = loadState(); return d; });

  function defaultState(full) {
    return {
      dish: data.dishes.map(() => ({ owned: full, level: full ? 2 : 1 })),
      character: data.characters.map(() => ({ owned: full, level: full ? 5 : 0 })),
      cafes: full ? MAX_CAFES : 1,
      popularity: full ? [3, 5, 5, 5, 5] : [0, 0, 0, 0, 0],
      trend: 'None',
      decor: !!full
    };
  }

  function loadState() {
    try {
      const saved = JSON.parse(localStorage.getItem(STATE_KEY) || 'null');
      if (saved && Array.isArray(saved.dish) && Array.isArray(saved.character))
        return { ...saved, trend: 'None' }; // trend is session-only
    } catch (_) { /* ignore broken local state */ }
    return defaultState(false);
  }

  function persist() {
    try { localStorage.setItem(STATE_KEY, JSON.stringify(state)); } catch (_) {}
  }

  const str = (o) => JSON.stringify(o);
  const ci = (a, b) => String(a || '').toLowerCase().includes(String(b || '').toLowerCase());
  const eq = (a, b) => String(a || '').toLowerCase() === String(b || '').toLowerCase();
  const clamp = (n, lo, hi) => Math.max(lo, Math.min(hi, Number(n) || 0));

  /* --- trends ----------------------------------------------------------- */
  function trendType(name) {
    if (!name || name === 'None') return 'none';
    if (['Main Dishes', 'Desserts', 'Beverages'].includes(name)) return 'tag';
    if (data.ingredients.some((i) => eq(i.category, name))) return 'category';
    return 'ingredient';
  }
  const trendBonus = (type) => type === 'tag' ? 1 : (type === 'category' || type === 'ingredient') ? 0.75 : 0;
  function trendTag(name) {
    if (name === 'Main Dishes') return 'main_dishes';
    if (name === 'Desserts') return 'desserts';
    if (name === 'Beverages') return 'beverages';
    return 'none';
  }
  const ingredientName = (id) => {
    const ing = data.ingredients.find((i) => i.id === id);
    return ing ? ing.name : '?';
  };
  function dishMatchesTrend(dish, name) {
    if (!name || name === 'None') return false;
    if (ci(dish.type, name)) return true;
    const ingredients = (dish.ingredients || []).map(ingredientName);
    if (ingredients.some((n) => ci(n, name))) return true;
    return (dish.ingredients || []).some((id) => {
      const ing = data.ingredients.find((i) => i.id === id);
      return ing && eq(ing.category, name);
    });
  }
  function dishTrendBonus(dish) {
    return dishMatchesTrend(dish, state.trend) ? (trendBonus(trendType(state.trend)) || 0) : 0;
  }
  function dishBasePrice(dish, level) {
    return (level === 2 ? dish.priceL2 : dish.priceL1) + dishTrendBonus(dish);
  }
  function popularityBonus() {
    let total = 0;
    for (let c = 0; c < Math.min(state.cafes, MAX_CAFES); c++) {
      const level = clamp(state.popularity[c], 0, TIERS[c].length);
      if (level > 0) total += TIERS[c][level - 1][1];
    }
    return total;
  }

  /* --- dish / character payloads ---------------------------------------- */
  function dishObject(dish, withState, i) {
    const obj = {
      name: dish.name,
      type: dish.type,
      priceL1: dish.priceL1,
      priceL2: dish.priceL2
    };
    if (withState) {
      obj.owned = !!state.dish[i].owned;
      obj.level = state.dish[i].level;
    }
    obj.ingredients = (dish.ingredients || []).map(ingredientName);
    return obj;
  }
  function characterObject(character, withState, i) {
    const obj = {
      name: character.name,
      skillCount: (character.skills || []).length,
      skills: (character.skills || []).map((s) => ({
        level: s.level,
        value: s.value,
        req: s.requirement,
        type: s.type,
        tag: s.tag
      }))
    };
    if (withState) {
      obj.owned = !!state.character[i].owned;
      obj.level = state.character[i].level;
    }
    return obj;
  }

  /* --- optimizer (port of lib/nte_cafe/src/{optimizer,evaluate,skills}.c) - */
  function menuStats(dishes, indices) {
    const counts = [0, 0, 0, 0];
    let basePriceSum = 0, trendBonusSum = 0;
    indices.forEach((idx) => {
      const d = dishes[idx];
      basePriceSum += d.basePrice;
      trendBonusSum += d.trendBonus;
      const t = d.dish.type === 'Beverages' ? 0 : d.dish.type === 'Desserts' ? 1 : d.dish.type === 'Main Dishes' ? 2 : 3;
      counts[t]++;
    });
    return { basePriceSum, trendBonusSum, counts, maxTypeCount: Math.max(...counts), dishCount: indices.length };
  }
  function skillActivates(skill, menu) {
    if (skill.tag === 'none') return true;
    const count = skill.tag === 'any' ? menu.maxTypeCount
      : skill.tag === 'beverages' ? menu.counts[0]
      : skill.tag === 'desserts' ? menu.counts[1]
      : skill.tag === 'main_dishes' ? menu.counts[2] : 0;
    return count >= skill.req;
  }
  function activeSkills(character, level) {
    return (character.skills || [])
      .filter((s) => s.requirement <= level && s.value > 0)
      .map((s) => ({ ...s, req: s.requirement }));
  }
  function incomeFast(menu, chars) {
    let traffic = 2400, priceBuff = 0;
    chars.forEach((ch) => ch.active.forEach((s) => {
      if (!skillActivates(s, menu)) return;
      if (s.type === 'traffic_flat') traffic += s.value;
      else if (s.type === 'price_flat') priceBuff += s.value;
    }));
    const base = traffic;
    let multBonus = 0, priceMult = 1;
    chars.forEach((ch) => ch.active.forEach((s) => {
      if (!skillActivates(s, menu)) return;
      if (s.type.startsWith('traffic') && s.type.includes('mult')) multBonus += base * s.value;
      else if (s.type.startsWith('price') && s.type.includes('mult')) priceMult += s.value;
    }));
    if (state.decor) multBonus += base * 0.01;
    traffic = base + multBonus;
    const adjusted = menu.basePriceSum - menu.trendBonusSum + menu.dishCount * priceBuff;
    return (adjusted * priceMult + menu.trendBonusSum) * (traffic / 100) * (1 + popularityBonus());
  }
  function nck(n, k) {
    if (k > n) return 0n;
    if (k > n - k) k = n - k;
    let r = 1n;
    for (let i = 1n; i <= BigInt(k); i++) r = (r * BigInt(n - k + Number(i))) / i;
    return r;
  }
  const MAX_U64 = 18446744073709551615n;
  const capped = (v) => (v > MAX_U64 ? MAX_U64 : v).toString();

  function optimize() {
    let dishSlots = clamp(state.cafes, 1, 5);
    let charSlots = Math.min(dishSlots * 2, 10);
    const dishes = data.dishes.map((d, i) => ({ dish: d, index: i, basePrice: dishBasePrice(d, state.dish[i].level), trendBonus: dishTrendBonus(d) }))
      .filter((d) => state.dish[d.index].owned);
    const chars = data.characters.map((c, i) => ({
      character: c, index: i, level: state.character[i].level,
      active: activeSkills(c, state.character[i].level)
    })).filter((c) => state.character[c.index].owned && c.level > 0);
    if (!dishes.length || !chars.length || dishes.length < dishSlots) return null;

    const groups = [[], [], [], []];
    dishes.forEach((d, idx) => {
      const t = d.dish.type === 'Beverages' ? 0 : d.dish.type === 'Desserts' ? 1 : d.dish.type === 'Main Dishes' ? 2 : 3;
      groups[t].push(idx);
    });
    groups.forEach((g) => g.sort((a, b) => dishes[b].basePrice - dishes[a].basePrice));
    if (charSlots > chars.length) charSlots = chars.length;

    let bestIncome = -1, bestDishes = null, bestChars = null;
    const charPick = new Array(charSlots);
    function scanChars(depth, next, menu, dishIdx) {
      if (depth === charSlots) {
        const picked = charPick.map((i) => chars[i]);
        const income = incomeFast(menu, picked);
        if (income > bestIncome) {
          bestIncome = income;
          bestDishes = dishIdx.slice();
          bestChars = charPick.slice();
        }
        return;
      }
      for (let i = next; i + (charSlots - depth) <= chars.length; i++) {
        charPick[depth] = i;
        scanChars(depth + 1, i + 1, menu, dishIdx);
      }
    }

    const c0max = Math.min(dishSlots, groups[0].length);
    for (let c0 = 0; c0 <= c0max; c0++) {
      const rem1 = dishSlots - c0;
      const cap1 = Math.min(rem1, groups[1].length);
      const min1 = Math.max(0, rem1 - groups[2].length - groups[3].length);
      for (let c1 = min1; c1 <= cap1; c1++) {
        const rem2 = rem1 - c1;
        const cap2 = Math.min(rem2, groups[2].length);
        const min2 = Math.max(0, rem2 - groups[3].length);
        for (let c2 = min2; c2 <= cap2; c2++) {
          const c3 = rem2 - c2;
          if (c3 > groups[3].length) continue;
          const idxs = [];
          [c0, c1, c2, c3].forEach((count, t) => { for (let k = 0; k < count; k++) idxs.push(groups[t][k]); });
          idxs.sort((a, b) => a - b);
          if (idxs.length !== dishSlots) continue;
          scanChars(0, 0, menuStats(dishes, idxs), idxs);
        }
      }
    }
    if (!bestDishes) return null;

    const bestMenu = menuStats(dishes, bestDishes);
    const pickedDishes = bestDishes.map((i) => dishes[i]);
    const pickedChars = bestChars.map((i) => chars[i]);
    let traffic = 2400, priceBuff = 0, priceMult = 1;
    pickedChars.forEach((ch) => ch.active.forEach((s) => {
      if (!skillActivates(s, bestMenu)) return;
      if (s.type === 'traffic_flat') traffic += s.value;
      else if (s.type === 'price_flat') priceBuff += s.value;
    }));
    const base = traffic;
    let multBonus = 0;
    pickedChars.forEach((ch) => ch.active.forEach((s) => {
      if (!skillActivates(s, bestMenu)) return;
      if (s.type.startsWith('traffic') && s.type.includes('mult')) multBonus += base * s.value;
      else if (s.type.startsWith('price') && s.type.includes('mult')) priceMult += s.value;
    }));
    if (state.decor) multBonus += base * 0.01;
    traffic = base + multBonus;
    const pop = popularityBonus();
    return {
      ok: true,
      income: ((bestMenu.basePriceSum - bestMenu.trendBonusSum + bestMenu.dishCount * priceBuff) * priceMult + bestMenu.trendBonusSum) * (traffic / 100) * (1 + pop),
      traffic,
      priceBuff,
      priceMultiplier: priceMult,
      trend: state.trend,
      trendBonus: trendBonus(trendType(state.trend)),
      popularityBonus: pop,
      dishes: pickedDishes.map((d) => {
        const inTrend = dishMatchesTrend(d.dish, state.trend);
        const tb = inTrend ? trendBonus(trendType(state.trend)) : 0;
        const fph = ((d.basePrice - tb + priceBuff) * priceMult + tb) * (traffic / 100) * (1 + pop);
        return { name: d.dish.name, level: state.dish[d.index].level, fonsPerHour: fph, inTrend };
      }),
      characters: pickedChars.map((c) => ({ name: c.character.name }))
    };
  }

  /* --- config.json (same ID-keyed format as the CLI) --------------------- */
  function toConfig() {
    const cfg = {
      format: 'nte-cafe-config',
      version: 1,
      cafes: state.cafes,
      trend: 'None', // session-only, like the C config
      decor: state.decor,
      popularity: state.popularity.slice(0, MAX_CAFES),
      characters: {},
      dishes: {}
    };
    data.characters.forEach((c, i) => { cfg.characters[c.id] = { owned: !!state.character[i].owned, level: state.character[i].level }; });
    data.dishes.forEach((d, i) => { cfg.dishes[d.id] = { owned: !!state.dish[i].owned, level: state.dish[i].level }; });
    return cfg;
  }
  function fromConfig(cfg) {
    const warn = [];
    data.characters.forEach((c, i) => {
      const e = cfg.characters[c.id];
      if (!e) { warn.push('unknown character ' + c.id); return; }
      state.character[i] = { owned: !!e.owned, level: clamp(e.level, 0, 5) };
    });
    data.dishes.forEach((d, i) => {
      const e = cfg.dishes[d.id];
      if (!e) { warn.push('unknown dish ' + d.id); return; }
      state.dish[i] = { owned: !!e.owned, level: clamp(e.level, 1, 2) };
    });
    state.cafes = clamp(cfg.cafes, 1, MAX_CAFES);
    state.popularity = Array.from({ length: MAX_CAFES }, (_, i) => clamp((cfg.popularity || [])[i], 0, TIERS[i].length));
    state.trend = 'None'; // configs never carry a trend
    state.decor = !!cfg.decor;
    persist();
    return warn.join('');
  }

  /* --- WebUI bindings (same names as webui_bind() in app/src/main.c) ------ */
  const bind = (name, fn) => {
    window[name] = async (...args) => { await ready; return str(fn(...args)); };
  };

  bind('getAppInfo', () => ({
    app: 'NTE Cafe Optimizer', webui: '2.5.0-beta.4 (site)',
    dishCount: data.dishes.length, characterCount: data.characters.length,
    ingredientCount: data.ingredients.length,
    gameVersion: data.gameVersion, contentHash: data.contentHash
  }));
  bind('getDishes', () => data.dishes.map((d) => dishObject(d, false)));
  bind('getDishStates', () => data.dishes.map((d, i) => dishObject(d, true, i)));
  bind('setDishState', (idx, owned, level) => {
    if (idx < 0 || idx >= data.dishes.length) return { ok: false, error: 'invalid index' };
    state.dish[idx] = { owned: !!owned, level: clamp(level, 1, 2) };
    persist();
    return { ok: true };
  });
  bind('setAllDishStates', (mode) => {
    state.dish.forEach((d, i) => {
      if (mode === 1) d.level = 2;
      else if (mode === 2 || mode === 5) d.level = 1;
      if (mode === 3) d.owned = true;
      else if (mode === 4 || mode === 5) d.owned = false;
    });
    persist();
    return { ok: true };
  });
  bind('getCharacters', () => data.characters.map((c) => characterObject(c, false)));
  bind('getCharacterStates', () => data.characters.map((c, i) => characterObject(c, true, i)));
  bind('setCharacterState', (idx, owned, level) => {
    if (idx < 0 || idx >= data.characters.length) return { ok: false, error: 'invalid index' };
    state.character[idx] = { owned: !!owned, level: clamp(level, 0, 5) };
    persist();
    return { ok: true };
  });
  bind('setAllCharacterStates', (mode) => {
    state.character.forEach((c) => {
      if (mode === 1) c.level = 5;
      else if (mode === 2 || mode === 5) c.level = 0;
      if (mode === 3) c.owned = true;
      else if (mode === 4 || mode === 5) c.owned = false;
    });
    persist();
    return { ok: true };
  });
  bind('getOptimizerSettings', () => ({
    trend: state.trend,
    hotoriCatDecor: state.decor,
    cafesOwned: state.cafes,
    trendBonus: trendBonus(trendType(state.trend)),
    trends: (() => {
      const names = ['None', 'Main Dishes', 'Desserts', 'Beverages'];
      data.ingredients.forEach((i) => { if (i.category && !names.includes(i.category)) names.push(i.category); });
      data.ingredients.forEach((i) => { if (i.name && !names.includes(i.name)) names.push(i.name); });
      return names.map((name) => ({ name, type: trendType(name), bonus: trendBonus(trendType(name)) }));
    })(),
    cafes: TIERS.map((tiers, index) => ({
      index, maxLevel: tiers.length, selected: state.popularity[index],
      bonus: (state.popularity[index] > 0 ? tiers[state.popularity[index] - 1][1] : 0),
      levels: tiers.map((t, i) => ({ level: i + 1, popularity: t[0], bonus: t[1] }))
    })),
    popularityBonus: popularityBonus()
  }));
  bind('setOptimizerSettings', (trend, hotori, cafes) => {
    state.trend = trend || 'None';
    state.decor = !!hotori;
    state.cafes = clamp(cafes, 1, MAX_CAFES);
    persist();
    return { ok: true };
  });
  bind('setCafePopularity', (idx, level) => {
    if (idx < 0 || idx >= MAX_CAFES) return { ok: false, error: 'invalid cafe index' };
    state.popularity[idx] = clamp(level, 0, TIERS[idx].length);
    persist();
    return { ok: true };
  });
  bind('setCafesOwned', (cafes) => {
    state.cafes = clamp(cafes, 1, MAX_CAFES);
    persist();
    return { ok: true };
  });
  bind('getOptimizerOverview', () => {
    const ownedChars = state.character.filter((c) => c.owned && c.level > 0).length;
    const ownedDishes = state.dish.filter((d) => d.owned).length;
    const l1 = state.dish.filter((d) => d.owned && d.level <= 1).length;
    const l2 = ownedDishes - l1;
    const dishSlots = Math.min(state.cafes, 5);
    const charSlots = Math.min(state.cafes * 2, 10);
    const dishCombos = nck(ownedDishes, dishSlots);
    const charCombos = nck(ownedChars, charSlots);
    const combos = dishCombos === 0n || charCombos === 0n ? 0n : dishCombos * charCombos;
    return {
      loadedCharacters: ownedChars, ownedDishes, ownedDishesL1: l1, ownedDishesL2: l2,
      dishSlots, characterSlots: charSlots,
      dishCombinations: capped(dishCombos), characterCombinations: capped(charCombos),
      possibleCombinations: capped(combos),
      isDefault: ownedChars === 0 && ownedDishes === 0 && state.cafes === 1 && !state.decor &&
        state.trend === 'None' && popularityBonus() <= 0
    };
  });
  bind('runOptimization', () => optimize() || {
    ok: false, error: 'Optimization failed. Check owned dishes/characters and cafe count.'
  });
  bind('getI18n', (lang) => (data.i18n && data.i18n[lang]) || {});
  bind('updateGamedata', (text) => {
    const parsed = JSON.parse(text);
    if (!parsed || parsed.format !== 'nte-cafe-gamedata') return { ok: false, error: 'invalid game data' };
    data = parsed;
    return { ok: true, gameVersion: parsed.gameVersion, contentHash: parsed.contentHash };
  });
  bind('exportConfig', () => toConfig());
  bind('importConfig', (text) => {
    const cfg = JSON.parse(text);
    if (!cfg || cfg.format !== 'nte-cafe-config' || !cfg.characters || !cfg.dishes)
      return { ok: false, error: 'invalid config' };
    return { ok: true, warning: fromConfig(cfg) };
  });
  bind('saveConfig', () => { persist(); return { ok: true }; });
  bind('loadConfig', () => {
    try {
      const saved = JSON.parse(localStorage.getItem(STATE_KEY) || 'null');
      if (!saved) return { ok: false, error: 'read failed' };
      state = saved;
      return { ok: true, warning: '' };
    } catch (_) { return { ok: false, error: 'read failed' }; }
  });
  bind('getConfigPath', () => ({ path: 'browser-local (localStorage)' }));
  bind('resetDefaults', () => {
    state = defaultState(false);
    persist();
    return { ok: true };
  });
})();
