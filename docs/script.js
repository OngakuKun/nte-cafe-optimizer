(() => {
  document.querySelectorAll('.copy').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const block = document.getElementById('code-' + btn.dataset.copy);
      const text = block ? block.innerText.trim() : '';
      if (!text) return;
      try {
        await navigator.clipboard.writeText(text);
        const original = btn.textContent;
        btn.textContent = 'Copied';
        setTimeout(() => { btn.textContent = original; }, 1600);
      } catch (_) {
        btn.textContent = 'Select manually';
      }
    });
  });
})();
