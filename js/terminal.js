/* ═══════════════════════════════════════════
   terminal.js — ToonTrad OCR pipeline demo
═══════════════════════════════════════════ */

'use strict';

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

const LINES_OCR = [
  { text: '$ python3 main_ocr.py \\',               color: '#89b4fa' },
  { text: "  '{\"engine\":\"auto\",\"language\":\"ja\"}'", color: '#89b4fa', delay: 40 },
  { text: '', instant: true },
  { text: '[OCRManager] Moteur: easyocr | Device: cpu', color: '#f5a623', delay: 150 },
  { text: "[OCRManager] Chargement 'easyocr'...",    color: '#888', delay: 50 },
  { text: '[tessdata]  TESSDATA_PREFIX ok',           color: '#888', delay: 400 },
  { text: '', instant: true },
  { text: '✓ OCR terminé — 7 blocs détectés',        color: '#27c93f', delay: 700 },
  { text: '', instant: true },
  { text: '[{"text": "こんにちは！", "conf": 0.94},',  color: '#a6e3a1', delay: 100 },
  { text: ' {"text": "行くぞ！",    "conf": 0.91}]',  color: '#a6e3a1' },
  { text: '', instant: true },
  { text: '$ _', color: '#89b4fa', cursor: true },
];

const LINES_NEWS = [
  { text: '$ toontrad --status',                     color: '#89b4fa' },
  { text: '', instant: true },
  { text: '  Projet    ToonTrad',                    color: '#cdd6f4', delay: 100 },
  { text: '  Branch    dev',                         color: '#cdd6f4' },
  { text: '  Version   v2.0.2-beta',                color: '#f5a623' },
  { text: '  Status    🚧 En développement actif',   color: '#27c93f' },
  { text: '------------------------------------------', delay: 10 },
  { text: '  [Pipeline] Error: .ttproject not found', color: '#e63b2e', delay: 150 },
  { text: '------------------------------------------', delay: 10 },
  { text: '  Moteurs   PaddleOCR · EasyOCR · TrOCR', color: '#888' },
  { text: '             manga-ocr · Tesseract',       color: '#888' },
  { text: '------------------------------------------', delay: 10 },
  { text: '  Stack     C++17 / Qt6 / Python / CMake', color: '#888' },
  { text: '------------------------------------------', delay: 10 },
  { text: '  → github.com/trotroni/toontrad',         color: '#e63b2e', delay: 200 },
  { text: '', instant: true },
  { text: '$ _', color: '#89b4fa', cursor: true },
];

async function typeLine(termBody, line) {
  if (line.delay) await sleep(line.delay);
  if (line.instant || line.text === '') {
    const div = document.createElement('div');
    div.style.color = line.color || '#cdd6f4';
    div.style.minHeight = '1lh';
    div.textContent = line.text;
    termBody.appendChild(div);
    termBody.scrollTop = termBody.scrollHeight;
    return;
  }

  const div = document.createElement('div');
  div.style.color = line.color || '#cdd6f4';
  div.style.minHeight = '1lh';
  const cur = document.createElement('span');
  cur.className = 'term-cursor';
  div.appendChild(cur);
  termBody.appendChild(div);

  for (const char of line.text) {
    div.insertBefore(document.createTextNode(char), cur);
    termBody.scrollTop = termBody.scrollHeight;
    let d = 22;
    if (char === ' ')            d = 8;
    if ('.,;:'.includes(char))  d = 70;
    await sleep(d + Math.random() * 14);
  }
  if (!line.cursor) cur.remove();
}

async function runTerminal(targetId, lines, startDelay = 10) {
  const termBody = document.getElementById(targetId);
  if (!termBody) return;
  await sleep(startDelay);
  for (const line of lines) {
    await typeLine(termBody, line);
    await sleep(25);
  }
}

// Lance dès que la section hero est visible
const termEl = document.querySelector('.hero-terminals');
if (termEl) {
  const obs = new IntersectionObserver(entries => {
    if (entries[0].isIntersecting) {
      runTerminal('termBody1', LINES_OCR,  900);   // commence à 0.9s
      runTerminal('termBody2', LINES_NEWS, 1800);  // commence à 1.8s
      obs.disconnect();
    }
  }, { threshold: 0.1 });
  obs.observe(termEl);
}
