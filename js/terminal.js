/* ═══════════════════════════════════════════
   terminal.js — ToonTrad OCR pipeline demo
═══════════════════════════════════════════ */

'use strict';

const termBody = document.getElementById('termBody');
if (!termBody) { throw new Error('termBody missing'); }

// Lines: { text, color?, delay?, instant?, cursor? }
const LINES = [
  { text: '$ python3 main_ocr.py \\',              color: '#89b4fa' },
  { text: "  '{\"engine\":\"auto\",\"language\":\"ja\",\"device\":\"auto\"}'", color: '#89b4fa', delay: 80 },
  { text: '', instant: true },
  { text: '[OCRManager] Moteur: easyocr | Device: cpu',  color: '#f5a623', delay: 300 },
  { text: "[OCRManager] Chargement du moteur 'easyocr'...", color: '#888', delay: 100 },
  { text: '[tesseract]  TESSDATA_PREFIX=/resources/tessdata', color: '#888', delay: 800 },
  { text: '', instant: true },
  { text: '✓ OCR terminé — 7 blocs détectés',       color: '#27c93f', delay: 1400 },
  { text: '', instant: true },
  { text: '// stdout → JSON',                        color: '#3a3a3a', instant: true },
  { text: '[',                                        color: '#cdd6f4', delay: 200 },
  { text: '  {"text": "こんにちは！", "confidence": 0.94, "box": [[12,8],...]},', color: '#a6e3a1', delay: 100 },
  { text: '  {"text": "行くぞ！",    "confidence": 0.91, "box": [[60,8],...]},', color: '#a6e3a1' },
  { text: '  {"text": "待ってくれ", "confidence": 0.88, "box": [[12,60],...]}',  color: '#a6e3a1' },
  { text: '  // + 4 autres blocs…',                  color: '#3a3a3a', instant: true },
  { text: ']',                                        color: '#cdd6f4' },
  { text: '', instant: true },
  { text: '$ _',                                     color: '#89b4fa', cursor: true },
];

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function appendLine(text, color) {
  const div = document.createElement('div');
  div.style.color = color || '#cdd6f4';
  div.style.minHeight = '1lh';
  div.textContent = text;
  termBody.appendChild(div);
  termBody.scrollTop = termBody.scrollHeight;
  return div;
}

async function typeLine(line) {
  if (line.delay) await sleep(line.delay);
  if (line.instant || line.text === '') { appendLine(line.text, line.color); return; }

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
    if ('.,;:'.includes(char)) d = 70;
    await sleep(d + Math.random() * 14);
  }
  if (!line.cursor) cur.remove();
}

async function runTerminal() {
  await sleep(900);
  for (const line of LINES) {
    await typeLine(line);
    await sleep(25);
  }
}

// Start only when visible
const termEl = document.querySelector('.hero-terminal');
if (termEl) {
  const obs = new IntersectionObserver(entries => {
    if (entries[0].isIntersecting) { runTerminal(); obs.disconnect(); }
  }, { threshold: 0.1 });
  obs.observe(termEl);
}
