(() => {
  /* Same theme switch as the app's settings panel, persisted locally. */
  const themeSelect = document.getElementById('themeSelect');
  const saved = localStorage.getItem('nte-site-theme') || 'macchiato';
  document.documentElement.dataset.theme = saved;
  if (themeSelect) {
    themeSelect.value = saved;
    themeSelect.addEventListener('change', () => {
      document.documentElement.dataset.theme = themeSelect.value;
      localStorage.setItem('nte-site-theme', themeSelect.value);
    });
  }

  /* The app keeps --header-h in sync with the fixed header height. */
  const updateHeaderH = () => {
    const h = document.querySelector('header');
    if (h) document.documentElement.style.setProperty('--header-h', h.offsetHeight + 'px');
  };
  updateHeaderH();
  window.addEventListener('resize', updateHeaderH);

  /* Mobile nav: same drawer/overlay behaviour as in the app. */
  const nav = document.getElementById('siteNav');
  const overlay = document.getElementById('navOverlay');
  const menuBtn = document.getElementById('menuBtn');
  const closeNav = () => {
    nav.classList.remove('open');
    overlay.classList.remove('open');
    menuBtn.setAttribute('aria-expanded', 'false');
  };
  if (nav && overlay && menuBtn) {
    menuBtn.addEventListener('click', () => {
      const open = !nav.classList.contains('open');
      nav.classList.toggle('open', open);
      overlay.classList.toggle('open', open);
      menuBtn.setAttribute('aria-expanded', String(open));
    });
    overlay.addEventListener('click', closeNav);
    nav.addEventListener('click', (e) => { if (e.target.closest('a')) closeNav(); });
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') closeNav(); });
  }

  /* Highlight the section currently in view, like the active nav link in the app. */
  const links = [...document.querySelectorAll('.nav-link')];
  const sections = links.map(a => document.querySelector(a.getAttribute('href'))).filter(Boolean);
  if ('IntersectionObserver' in window && sections.length) {
    const observer = new IntersectionObserver((entries) => {
      const visible = entries.filter(e => e.isIntersecting).sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
      if (!visible) return;
      links.forEach(a => a.classList.toggle('active', a.getAttribute('href') === '#' + visible.target.id));
    }, { rootMargin: '-20% 0px -60% 0px', threshold: [0, .25, .5] });
    sections.forEach(s => observer.observe(s));
  }

  /* Copy buttons for the quick-start commands. */
  document.querySelectorAll('.copy-btn, #copyDocker').forEach(btn => {
    btn.addEventListener('click', async () => {
      const id = btn.dataset.copy || 'dockerCmd';
      const block = document.getElementById(id);
      const text = block ? block.innerText.trim() : '';
      if (!text) return;
      const original = btn.textContent;
      try {
        await navigator.clipboard.writeText(text);
        btn.textContent = 'Copied';
      } catch (_) {
        btn.textContent = 'Select manually';
      }
      setTimeout(() => { btn.textContent = original; }, 1600);
    });
  });
})();
