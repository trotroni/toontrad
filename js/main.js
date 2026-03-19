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

// ── Theme toggle ─────────────────────────
const themeToggle = document.getElementById('themeToggle');
const THEME_KEY   = 'toontrad-theme';

// Restore saved preference
if (localStorage.getItem(THEME_KEY) === 'light') {
  document.body.classList.add('light');
}

if (themeToggle) {
  themeToggle.addEventListener('click', () => {
    const isLight = document.body.classList.toggle('light');
    localStorage.setItem(THEME_KEY, isLight ? 'light' : 'dark');
  });
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