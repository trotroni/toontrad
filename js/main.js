/* ═══════════════════════════════════════════
   main.js — ToonTrad project page
   Cursor · Nav · Scroll reveal · Mobile burger
═══════════════════════════════════════════ */

'use strict';

// ── Custom cursor ─────────────────────────
const cursor = document.getElementById('cursor');
if (cursor && window.matchMedia('(hover: hover)').matches) {
  let mx = 0, my = 0, cx = 0, cy = 0;
  document.addEventListener('mousemove', e => { mx = e.clientX; my = e.clientY; });
  (function loop() {
    cx += (mx - cx) * 0.18;
    cy += (my - cy) * 0.18;
    cursor.style.left = cx + 'px';
    cursor.style.top  = cy + 'px';
    requestAnimationFrame(loop);
  })();
  document.querySelectorAll('a, button, .pipe-card, .engine-row, .comp-card, .arch-box').forEach(el => {
    el.addEventListener('mouseenter', () => cursor.classList.add('hover'));
    el.addEventListener('mouseleave', () => cursor.classList.remove('hover'));
  });
}

// ── Nav scroll ───────────────────────────
const nav = document.getElementById('nav');
window.addEventListener('scroll', () => {
  nav.classList.toggle('scrolled', window.scrollY > 40);
  updateActiveLink();
}, { passive: true });

function updateActiveLink() {
  const scrollY = window.scrollY + 100;
  document.querySelectorAll('section[id]').forEach(sec => {
    const link = document.querySelector(`.nav-link[href="#${sec.id}"]`);
    if (!link) return;
    if (scrollY >= sec.offsetTop && scrollY < sec.offsetTop + sec.offsetHeight) {
      document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
      link.classList.add('active');
    }
  });
}

// ── Mobile burger ────────────────────────
const navBurger = document.getElementById('navBurger');
const navLinks  = document.getElementById('navLinks');
if (navBurger && navLinks) {
  navBurger.addEventListener('click', () => {
    const open = navLinks.classList.toggle('open');
    navBurger.classList.toggle('open', open);
  });
  navLinks.querySelectorAll('a').forEach(a => {
    a.addEventListener('click', () => {
      navLinks.classList.remove('open');
      navBurger.classList.remove('open');
    });
  });
}

// ── Scroll reveal ─────────────────────────
const revealObs = new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      entry.target.classList.add('visible');
      revealObs.unobserve(entry.target);
    }
  });
}, { threshold: 0.1 });

document.querySelectorAll('.reveal, .reveal-right').forEach(el => revealObs.observe(el));


// ── Project counter ──────────────────────
function pad(n) { return String(n).padStart(2, '0'); }

let PROJECT_START = null;
let counterInterval = null;

function updateCounter() {
  if (!PROJECT_START) return;
  const diff = Math.max(0, new Date() - PROJECT_START);

  const totalSec  = Math.floor(diff / 1000);
  const seconds   = totalSec % 60;
  const totalMin  = Math.floor(totalSec / 60);
  const minutes   = totalMin % 60;
  const totalHour = Math.floor(totalMin / 60);
  const hours     = totalHour % 24;
  const days      = Math.floor(totalHour / 24);

  const set = (id, val) => { const el = document.getElementById(id); if (el) el.textContent = val; };
  set('cDays',    days);
  set('cHours',   pad(hours));
  set('cMinutes', pad(minutes));
  set('cSeconds', pad(seconds));
}

// Fetch repo creation date from GitHub API
fetch('https://api.github.com/repos/trotroni/toontrad')
  .then(r => r.json())
  .then(data => {
    if (data.created_at) {
      PROJECT_START = new Date(data.created_at);
      updateCounter();
      counterInterval = setInterval(updateCounter, 1000);
    }
  })
  .catch(() => {
    // Fallback: use repo creation date from last known API response
    PROJECT_START = new Date('2026-02-27T22:42:53Z');
    updateCounter();
    counterInterval = setInterval(updateCounter, 1000);
  });

// ── Theme toggle (dark → grey → light → dark) ──
const themeToggle = document.getElementById('themeToggle');
const themeIcon   = document.getElementById('themeIcon');
const THEME_KEY   = 'toontrad-theme';
const THEMES      = ['dark', 'grey', 'light'];
const ICONS       = { dark: '☽', grey: '◑', light: '☀' };

let currentTheme = localStorage.getItem(THEME_KEY) || 'grey';
applyTheme(currentTheme);

if (themeToggle) {
  themeToggle.addEventListener('click', () => {
    const next = THEMES[(THEMES.indexOf(currentTheme) + 1) % THEMES.length];
    applyTheme(next);
    localStorage.setItem(THEME_KEY, next);
  });
}

function applyTheme(theme) {
  document.body.classList.remove('light', 'grey');
  if (theme !== 'dark') document.body.classList.add(theme);
  if (themeIcon) themeIcon.textContent = ICONS[theme];
  currentTheme = theme;
}

document.querySelectorAll('a[href^="#"]').forEach(a => {
  a.addEventListener('click', e => {
    const target = document.querySelector(a.getAttribute('href'));
    if (!target) return;
    e.preventDefault();
    const offset = 60;
    window.scrollTo({ top: target.offsetTop - offset, behavior: 'smooth' });
  });
});

// ── Releases ─────────────────────────────
function formatBytes(bytes) {
  if (bytes < 1024)       return bytes + ' B';
  if (bytes < 1024*1024)  return (bytes/1024).toFixed(0) + ' KB';
  return (bytes/1024/1024).toFixed(1) + ' MB';
}

function formatDate(iso) {
  return new Date(iso).toLocaleDateString('fr-FR', {
    day: 'numeric', month: 'long', year: 'numeric'
  });
}

function dlIcon() {
  return `<svg width="12" height="12" viewBox="0 0 16 16" fill="currentColor"><path d="M8 12l-4-4h2.5V3h3v5H12L8 12zm-6 2h12v1.5H2V14z"/></svg>`;
}

function buildReleaseCard(release, isLatest) {
  const assets = release.assets || [];
  const notes  = (release.body || '').trim();
  const tagStr = release.tag_name || 'v?';

  const assetsHTML = assets.length
    ? `<div class="release-assets-title">Assets (${assets.length})</div>
       <div class="release-assets">
         ${assets.map(a => `
           <div class="asset-row">
             <div class="asset-info">
               <span class="asset-name">${a.name}</span>
               <span class="asset-size">${formatBytes(a.size)} · ${a.download_count} téléchargement${a.download_count !== 1 ? 's' : ''}</span>
             </div>
             <a class="asset-dl" href="${a.browser_download_url}" download>
               ${dlIcon()} Télécharger
             </a>
           </div>`).join('')}
       </div>`
    : `<div class="release-no-assets">Aucun asset joint à cette release.</div>`;

  const notesHTML = notes
    ? `<div class="release-notes release-md">${
        typeof marked !== 'undefined'
          ? marked.parse(notes, { breaks: true, gfm: true })
          : notes.replace(/</g,'&lt;').replace(/>/g,'&gt;')
      }</div>`
    : '';

  const card = document.createElement('div');
  card.className = 'release-card' + (isLatest ? ' latest' : '');
  card.innerHTML = `
    <div class="release-header">
      <span class="release-tag">${tagStr}</span>
      <span class="release-name">${release.name || tagStr}</span>
      <span class="release-date">${formatDate(release.published_at)}</span>
      <span class="release-toggle">▼</span>
    </div>
    <div class="release-body">
      ${notesHTML}
      ${assetsHTML}
      <a class="release-gh-link" href="${release.html_url}" target="_blank" rel="noopener">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0 0 24 12c0-6.63-5.37-12-12-12z"/></svg>
        Voir sur GitHub
      </a>
    </div>`;

  // Toggle accordion
  card.querySelector('.release-header').addEventListener('click', () => {
    card.classList.toggle('open');
  });

  // Auto-open latest
  if (isLatest) card.classList.add('open');

  return card;
}

function loadReleases() {
  const elLoading = document.getElementById('releases-loading');
  const elEmpty   = document.getElementById('releases-empty');
  const elError   = document.getElementById('releases-error');
  const elList    = document.getElementById('releases-list');
  if (!elList) return;

  fetch('https://api.github.com/repos/trotroni/toontrad/releases')
    .then(r => {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    })
    .then(releases => {
      elLoading.style.display = 'none';

      if (!releases.length) {
        elEmpty.style.display = 'block';
        return;
      }

      elList.style.display = 'block';
      releases.forEach((release, i) => {
        elList.appendChild(buildReleaseCard(release, i === 0));
      });
    })
    .catch(() => {
      elLoading.style.display = 'none';
      elError.style.display = 'block';
    });
}

// Lazy load releases when section scrolls into view
const releasesSection = document.getElementById('releases');
if (releasesSection) {
  const relObs = new IntersectionObserver(entries => {
    if (entries[0].isIntersecting) {
      loadReleases();
      relObs.disconnect();
    }
  }, { threshold: 0.1 });
  relObs.observe(releasesSection);
}

// ── Last commit ──────────────────────────
(function () {
  const el = document.getElementById('lastCommit');
  if (!el) return;

  fetch('https://api.github.com/repos/trotroni/toontrad/commits/dev')
    .then(r => r.ok ? r.json() : Promise.reject())
    .then(data => {
      const sha     = data.sha.slice(0, 7);
      const date    = new Date(data.commit.author.date);
      const ago     = timeAgo(date);
      const msg     = data.commit.message.split('\n')[0].slice(0, 48);
      el.innerHTML  =
        `<a href="${data.html_url}" target="_blank" rel="noopener" class="commit-link">`
        + `<span class="commit-sha">${sha}</span>`
        + `<span class="commit-sep">·</span>`
        + `<span class="commit-msg">${msg}</span>`
        + `<span class="commit-sep">·</span>`
        + `<span class="commit-ago">${ago}</span>`
        + `</a>`;
    })
    .catch(() => {
      el.textContent = 'dev';
    });

  function timeAgo(date) {
    const s = Math.floor((Date.now() - date) / 1000);
    if (s < 60)   return 'à l\'instant';
    if (s < 3600) return `il y a ${Math.floor(s/60)} min`;
    if (s < 86400)return `il y a ${Math.floor(s/3600)} h`;
    const d = Math.floor(s/86400);
    return `il y a ${d} jour${d > 1 ? 's' : ''}`;
  }
})();
