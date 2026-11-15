// ===== OLED drawing core: 32x128 portrait, 1-bit framebuffer =====
const W = 32, H = 128;

function makeScreen() { return { buf: new Uint8Array(W * H), top: 0, bot: H }; }
function clear(g) { g.buf.fill(0); g.top = 0; g.bot = H; }
function clip(g, top, bot) { g.top = top; g.bot = bot; }
function px(g, x, y, v = 1) {
  x = Math.round(x); y = Math.round(y);
  if (x < 0 || x >= W || y < g.top || y >= g.bot || y < 0 || y >= H) return;
  g.buf[y * W + x] = v;
}
function frect(g, x, y, w, h, v = 1) { for (let i = 0; i < w; i++) for (let j = 0; j < h; j++) px(g, x + i, y + j, v); }
function rect(g, x, y, w, h, v = 1) {
  for (let i = 0; i < w; i++) { px(g, x + i, y, v); px(g, x + i, y + h - 1, v); }
  for (let j = 0; j < h; j++) { px(g, x, y + j, v); px(g, x + w - 1, y + j, v); }
}
function line(g, x0, y0, x1, y1, v = 1) {
  x0 = Math.round(x0); y0 = Math.round(y0); x1 = Math.round(x1); y1 = Math.round(y1);
  const dx = Math.abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -Math.abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  let e = dx + dy;
  for (;;) { px(g, x0, y0, v); if (x0 === x1 && y0 === y1) break; const e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
}
// Disk with a float centre, so it can sit symmetrically on a 2-px stem.
function disk(g, cx, cy, r, v = 1) {
  for (let y = Math.floor(cy - r - 1); y <= Math.ceil(cy + r + 1); y++)
    for (let x = Math.floor(cx - r - 1); x <= Math.ceil(cx + r + 1); x++) {
      const dx = x - cx, dy = y - cy;
      if (dx * dx + dy * dy <= r * r + 0.6 * r) px(g, x, y, v);
    }
}
function ring(g, cx, cy, r, v = 1) {
  for (let y = Math.floor(cy - r - 1); y <= Math.ceil(cy + r + 1); y++)
    for (let x = Math.floor(cx - r - 1); x <= Math.ceil(cx + r + 1); x++) {
      const d = Math.hypot(x - cx, y - cy);
      if (Math.abs(d - r) < 0.55) px(g, x, y, v);
    }
}
function tri(g, a, b, c, v = 1) {
  const minx = Math.floor(Math.min(a[0], b[0], c[0])), maxx = Math.ceil(Math.max(a[0], b[0], c[0]));
  const miny = Math.floor(Math.min(a[1], b[1], c[1])), maxy = Math.ceil(Math.max(a[1], b[1], c[1]));
  const cr = (p, q, r) => (p[0] - r[0]) * (q[1] - r[1]) - (q[0] - r[0]) * (p[1] - r[1]);
  for (let y = miny; y <= maxy; y++) for (let x = minx; x <= maxx; x++) {
    const p = [x, y], d1 = cr(p, a, b), d2 = cr(p, b, c), d3 = cr(p, c, a);
    const neg = d1 < -0.5 || d2 < -0.5 || d3 < -0.5, pos = d1 > 0.5 || d2 > 0.5 || d3 > 0.5;
    if (!(neg && pos)) px(g, x, y, v);
  }
}

// 3x5 font: one row = 3 bits, MSB on the left.
const FONT = {
  A: [2, 5, 7, 5, 5], B: [6, 5, 6, 5, 6], D: [6, 5, 5, 5, 6], E: [7, 4, 6, 4, 7], G: [3, 4, 5, 5, 3],
  H: [5, 5, 7, 5, 5], I: [7, 2, 2, 2, 7], J: [1, 1, 1, 5, 2], L: [4, 4, 4, 4, 7], M: [5, 7, 7, 5, 5],
  N: [6, 5, 5, 5, 5], O: [2, 5, 5, 5, 2], P: [6, 5, 6, 4, 4], R: [6, 5, 6, 5, 5], S: [3, 4, 2, 1, 6],
  T: [7, 2, 2, 2, 2], U: [5, 5, 5, 5, 7], W: [5, 5, 5, 7, 5], Z: [7, 1, 2, 4, 7], '!': [2, 2, 2, 0, 2],
  ' ': [0, 0, 0, 0, 0],
  0: [7, 5, 5, 5, 7], 1: [2, 6, 2, 2, 7], 2: [6, 1, 2, 4, 7], 3: [6, 1, 2, 1, 6], 4: [5, 5, 7, 1, 1],
  5: [7, 4, 6, 1, 6], 6: [3, 4, 6, 5, 2], 7: [7, 1, 2, 2, 2], 8: [2, 5, 2, 5, 2], 9: [2, 5, 3, 1, 6],
};
function glyph(g, ch, x, y, v = 1, scale = 1) {
  const f = FONT[ch]; if (!f) return;
  for (let r = 0; r < 5; r++) for (let b = 0; b < 3; b++) if ((f[r] >> (2 - b)) & 1) frect(g, x + b * scale, y + r * scale, scale, scale, v);
}
function text(g, s, y, x) {
  if (x === undefined) x = Math.floor((W - (s.length * 4 - 1)) / 2);
  for (const c of s) { glyph(g, c, x, y); x += 4; }
}

// ===== Layers =====
const LAYERS = ['BASE', 'LOWER', 'RAISE', 'ADJUST', 'HARDER', 'MIRROR', 'LMIRROR', 'GAMING', 'LGAMING'];
const FAMILY = { BASE: 'main', LOWER: 'main', RAISE: 'main', ADJUST: 'main', HARDER: 'main',
  MIRROR: 'mirror', LMIRROR: 'mirror', GAMING: 'gaming', LGAMING: 'gaming' };
const STACK = { main: ['RAISE', 'BASE', 'LOWER'], mirror: [null, 'MIRROR', 'LMIRROR'], gaming: [null, 'GAMING', 'LGAMING'] };
const tri6 = t => [0, 1, 2, 3, 2, 1][t % 6];

function hud(g, layer) {
  text(g, layer, 3);
  for (let x = 2; x < 30; x += 2) px(g, x, 11);
}
function stack(g, layer, t) {
  const rows = STACK[FAMILY[layer]];
  const act = layer === 'ADJUST' ? ['RAISE', 'LOWER'] : layer === 'HARDER' ? ['RAISE', 'BASE', 'LOWER'] : [layer];
  rows.forEach((name, i) => {
    const y = 66 + i * 16;
    if (!name) { for (let x = 4; x < 28; x += 2) { px(g, x, y); px(g, x, y + 9); } return; }
    rect(g, 4, y, 24, 10);
    if (!act.includes(name)) return;
    if (layer === 'HARDER' && t % 4 >= 2) return;            // blinking stack
    frect(g, 4, y, 24, 10);
    frect(g, 4 + (t * 2) % 24, y + 1, 2, 8, 0);              // moving glint
  });
}
function gauge(g, wpm) { rect(g, 2, 118, 28, 6); frect(g, 4, 120, Math.round(24 * Math.min(wpm, 120) / 120), 2); }

// --- Variant A icons (zone y 14..60, centre y 37) ---
function varrow(g, c, top, up) {
  for (let i = 0; i < 6; i++) { const y = up ? top + i : top + 11 - i; for (let k = -i; k <= i; k++) px(g, c + k, y); }
  frect(g, c - 1, up ? top + 6 : top, 3, 6);
}
function harrow(g, tip, cy, dir) {          // dir -1 = points left
  for (let i = 0; i < 5; i++) for (let k = -i; k <= i; k++) px(g, tip - dir * i, cy + k);
  frect(g, dir < 0 ? tip + 5 : tip - 9, cy - 1, 5, 3);
}
// Gamepad in line art, 26x13, angular, with shoulder buttons, drawn at (x0, y0). k = which d-pad arm / face button is pressed.
const PAD = [
  '...####............####...',
  '..######################..',
  '.#......................#.',
  '#........................#',
  '#........................#',
  '#........................#',
  '#........................#',
  '#........................#',
  '#.......##########.......#',
  '#......#..........#......#',
  '#.....#............#.....#',
  '.#...#..............#...#.',
  '..###................###..',
];
function gamepad(g, y0, k) {
  const x0 = 3;
  PAD.forEach((row, j) => { for (let i = 0; i < row.length; i++) if (row[i] === '#') px(g, x0 + i, y0 + j); });
  // d-pad: filled plus, the pressed arm pulls in
  const dx = x0 + 6, dy = y0 + 5;
  frect(g, dx - 2, dy, 5, 1); frect(g, dx, dy - 2, 1, 5);
  const arm = [[0, -2], [2, 0], [0, 2], [-2, 0]][k]; px(g, dx + arm[0], dy + arm[1], 0);
  // face buttons: diamond of dots, the pressed one flashes as a small plus
  const bx = x0 + 20, by = y0 + 5;
  [[0, -2], [2, 0], [0, 2], [-2, 0]].forEach(([ox, oy], i) => {
    const x = bx + ox, y = by + oy; px(g, x, y);
    if (i === (k + 1) % 4) { px(g, x - 1, y); px(g, x + 1, y); px(g, x, y - 1); px(g, x, y + 1); }
  });
  frect(g, x0 + 10, y0 + 5, 2, 1); frect(g, x0 + 14, y0 + 5, 2, 1);   // select / start
}
function iconA(g, layer, t) {
  clip(g, 14, 60);
  const o = t % 16;
  switch (layer) {
    case 'BASE': { const r = Math.abs((t % 12) - 6);
      for (let dx = -r; dx <= r; dx++) { const dy = r - Math.abs(dx); px(g, 16 + dx, 37 + dy); px(g, 16 + dx, 37 - dy); } break; }
    case 'RAISE': for (let k = -1; k < 4; k++) varrow(g, 16, 14 + k * 16 - o, true); break;
    case 'LOWER': for (let k = -1; k < 4; k++) varrow(g, 16, 14 + k * 16 + o - 16, false); break;
    case 'ADJUST': for (let k = -1; k < 4; k++) { varrow(g, 9, 14 + k * 16 - o, true); varrow(g, 22, 14 + k * 16 + o - 16, false); } break;
    case 'HARDER': {
      const on = t % 4 < 2;
      tri(g, [15.5, 17], [3, 56], [28, 56]);
      if (!on) tri(g, [15.5, 23], [7, 53], [24, 53], 0);
      frect(g, 15, 30, 2, 13, on ? 0 : 1); frect(g, 15, 46, 2, 3, on ? 0 : 1);
      break; }
    case 'MIRROR': case 'LMIRROR': {
      const s = 3 - tri6(t), cy = layer === 'MIRROR' ? 37 : 31;
      for (let y = 14; y < 60; y += 4) { frect(g, 15, y, 2, 2); }
      harrow(g, 2 + s, cy, -1); harrow(g, 29 - s, cy, 1);
      if (layer === 'LMIRROR') varrow(g, 16, 41 + (t % 4), false);
      break; }
    case 'GAMING': case 'LGAMING': {
      const y = layer === 'GAMING' ? 30 : 26, k = t % 4;
      gamepad(g, y, k);
      if (layer === 'LGAMING') varrow(g, 16, 45 + (t % 4), false);
      break; }
  }
  clip(g, 0, H);
}

// ===== Mascot: "Lily", a little lily flower =====
// o.cx: centre between the two stem columns (cx-1, cx); o.ground: feet row.
function leaf(g, x0, y0, dx, dy) {
  line(g, x0, y0, x0 + dx, y0 + dy);
  const k = v => Math.sign(v) * Math.round(Math.abs(v) * 0.7);
  line(g, x0, y0 + 1, x0 + k(dx), y0 + 1 + k(dy));
}
function petal(g, cx, cy, a, R) {
  const P = (ang, d) => [cx + Math.cos(ang) * d, cy + Math.sin(ang) * d];
  tri(g, P(a, R + 7), P(a - 0.55, R - 1), P(a + 0.55, R - 1));
}
function lily(g, o) {
  const R = 7, stem = Math.max(2, 6 - o.squash + o.stretch);
  const gy = o.ground - o.lift, cx = o.cx + o.bx;
  const hcx = cx + o.hx, hcy = gy - stem - R - 1;   // head centre is hcx - 0.5
  const fc = hcx - 0.5;
  // petals, then the head with a black gap ring
  for (const a of o.angles) petal(g, fc, hcy, a, R);
  disk(g, fc, hcy, R + 1, 0); disk(g, fc, hcy, R, 1);
  // stem + feet, then leaves (arms), drawn over the gap ring so they connect
  frect(g, cx - 1, hcy + R, 2, gy - (hcy + R));
  frect(g, cx - 3, gy, 2, 1); frect(g, cx + 1, gy, 2, 1);
  const ay = hcy + R + 1 + Math.floor(stem / 3);
  const E = { down: [4, 3], up: [6, -4], out: [5, -1] };
  leaf(g, cx - 2, ay, -E[o.arms[0]][0], E[o.arms[0]][1]);
  leaf(g, cx + 1, ay, E[o.arms[1]][0], E[o.arms[1]][1]);
  // headband (gaming)
  if (o.band) {
    frect(g, hcx - 7, hcy - 4, 14, 1, 0);
    const f = o.t % 2;
    px(g, hcx + 7, hcy - 4); px(g, hcx + 8, hcy - 4 + f); px(g, hcx + 9, hcy - 3 + f); px(g, hcx + 10, hcy - 3 + 2 * f);
  }
  // eyes (black on the white face), symmetric around fc
  const [lx, ly] = o.look, L = hcx - 4 + lx, Rr = hcx + 2 + lx, ey = hcy - 2 + ly;
  switch (o.eyes) {
    case 'open': frect(g, L, ey, 2, 3, 0); frect(g, Rr, ey, 2, 3, 0); break;
    case 'closed': frect(g, L - 1, ey + 1, 3, 1, 0); frect(g, Rr, ey + 1, 3, 1, 0); break;
    case 'happy':
      px(g, L - 1, ey + 2, 0); px(g, L, ey + 1, 0); px(g, L + 1, ey + 2, 0);
      px(g, Rr, ey + 2, 0); px(g, Rr + 1, ey + 1, 0); px(g, Rr + 2, ey + 2, 0); break;
    case 'angry':
      px(g, hcx - 6, hcy - 4, 0); px(g, hcx - 5, hcy - 3, 0); px(g, hcx - 4, hcy - 2, 0);
      px(g, hcx + 5, hcy - 4, 0); px(g, hcx + 4, hcy - 3, 0); px(g, hcx + 3, hcy - 2, 0);
      frect(g, hcx - 4, hcy - 1, 2, 2, 0); frect(g, hcx + 2, hcy - 1, 2, 2, 0); break;
    case 'focus':
      frect(g, L - 1, ey, 3, 1, 0); frect(g, Rr, ey, 3, 1, 0);
      frect(g, L, ey + 1, 2, 2, 0); frect(g, Rr, ey + 1, 2, 2, 0); break;
  }
  // mouth
  switch (o.mouth) {
    case 'small': frect(g, hcx - 1, hcy + 3, 2, 1, 0); break;
    case 'smile': px(g, hcx - 2, hcy + 2, 0); frect(g, hcx - 1, hcy + 3, 2, 1, 0); px(g, hcx + 1, hcy + 2, 0); break;
    case 'open': frect(g, hcx - 1, hcy + 3, 2, 2, 0); break;
  }
  return { hcx, hcy, top: hcy - R - 6 };
}

function poseFor(layer, t, wpm, sleeping) {
  const up = -Math.PI / 2, d = 0.9;
  const p = { t, squash: 0, stretch: 0, lift: 0, bx: 0, hx: 0, arms: ['down', 'down'], eyes: 'open',
    look: [0, 0], mouth: 'small', angles: [up - d, up, up + d], band: false, fx: [], spin: 0, sleep: !!sleeping };
  if (sleeping) {
    Object.assign(p, { eyes: 'closed', mouth: 'small', angles: [up - d - 0.5, up, up + d + 0.5], hx: 0 });
    p.fx.push('zzz'); return p;
  }
  const fam = FAMILY[layer], low = layer === 'LOWER' || layer === 'LMIRROR' || layer === 'LGAMING';
  if (low) { p.squash = 4; p.eyes = 'open'; p.look = [0, 1]; p.arms = ['out', 'out']; p.lift = 0; p.hx = 0; p.bx = 0; }
  switch (layer) {
    case 'BASE':
      p.hx = [0, 0, 1, 1, 0, 0, -1, -1][t % 8 >> 0 & 7] || 0;
      if (t % 24 === 0 || t % 24 === 1) p.eyes = 'closed';
      p.mouth = 'smile'; break;
    case 'LOWER': p.lift = 0; p.squash = 4 + (t % 4 < 2 ? 0 : 1); break;
    case 'RAISE': p.arms = ['up', 'up']; p.stretch = 2; p.lift = [0, 2, 4, 2][t % 4]; p.mouth = 'open'; p.look = [0, -1]; break;
    case 'ADJUST': p.eyes = 'happy'; p.mouth = 'smile'; p.arms = ['out', 'out'];
      p.angles = [0, 1, 2].map(k => up + t * 0.35 + k * 2 * Math.PI / 3); p.spin = t * 0.35; p.fx.push('sparkle'); break;
    case 'HARDER': p.eyes = 'angry'; p.mouth = 'open'; p.arms = ['up', 'up']; p.bx = t % 2 ? 1 : -1; p.fx.push('bang'); break;
  }
  if (fam === 'mirror') {
    const w = t % 8 < 4;
    p.arms = low ? (w ? ['up', 'out'] : ['out', 'up']) : (w ? ['up', 'down'] : ['down', 'up']);
    p.look = [w ? -1 : 1, low ? 1 : 0]; p.fx.push('mirror');
  }
  if (fam === 'gaming') {
    p.pad = true; p.look = [0, 1]; p.eyes = 'focus'; p.mouth = 'small'; p.arms = ['out', 'out'];
    if (!low) p.lift = t % 4 === 0 ? 1 : 0;
  }
  if (wpm > 80 && layer !== 'HARDER') { p.mouth = 'open'; p.fx.push('sweat'); }
  return p;
}
function drawLily(g, layer, t, wpm, sleeping, cx, ground, top, style) {
  const p = poseFor(layer, t, wpm, sleeping);
  if (p.fx.includes('mirror')) for (let y = top; y < ground; y += 4) frect(g, 15, y, 2, 2);
  const h = (STYLES[style] || lily)(g, Object.assign({ cx, ground }, p));
  if (p.fx.includes('bang') && t % 2 === 0) { frect(g, h.hcx - 1, h.top - 9, 2, 6); frect(g, h.hcx - 1, h.top - 2, 2, 2); }
  if (p.fx.includes('sparkle')) {
    const pts = [[-11, -6], [10, -2], [-10, 4], [11, 7]];
    pts.forEach(([dx, dy], i) => { if ((t + i) % 3 === 0) { const x = h.hcx + dx, y = h.hcy + dy; px(g, x, y); px(g, x - 1, y); px(g, x + 1, y); px(g, x, y - 1); px(g, x, y + 1); } });
  }
  if (p.fx.includes('sweat')) { const y = h.hcy - 3 + (t % 4); px(g, h.hcx + 8, y); px(g, h.hcx + 8, y + 1); px(g, h.hcx + 9, y + 1); }
  if (p.fx.includes('zzz')) {   // a Z rises from behind the head, two more follow higher up
    const k = t % 18;
    glyph(g, 'Z', h.hcx + 5, h.hcy - 18 - (k >> 1));
    if (k > 5) glyph(g, 'Z', h.hcx + 9, h.hcy - 24 - ((k - 6) >> 1));
    if (k > 11) glyph(g, 'Z', h.hcx + 13, h.hcy - 30 - ((k - 12) >> 1));
  }
}


// ===== Lily, new styles =====
// Face features in a given ink (0 on a filled face, 1 on a line-art face).
function face(g, hcx, hcy, o, ink, f) {
  const sp = f.sp, eh = f.eh, [lx, ly] = o.look;
  const L = hcx - 2 - sp + lx, Rr = hcx + sp + lx, ey = hcy - 2 + ly;
  switch (o.eyes) {
    case 'open': frect(g, L, ey, 2, eh, ink); frect(g, Rr, ey, 2, eh, ink); break;
    case 'closed': frect(g, L - 1, ey + 1, 3, 1, ink); frect(g, Rr, ey + 1, 3, 1, ink); break;
    case 'happy':
      px(g, L - 1, ey + 2, ink); px(g, L, ey + 1, ink); px(g, L + 1, ey + 2, ink);
      px(g, Rr, ey + 2, ink); px(g, Rr + 1, ey + 1, ink); px(g, Rr + 2, ey + 2, ink); break;
    case 'angry':
      px(g, L - 2, ey - 2, ink); px(g, L - 1, ey - 1, ink); px(g, L, ey, ink);
      px(g, Rr + 3, ey - 2, ink); px(g, Rr + 2, ey - 1, ink); px(g, Rr + 1, ey, ink);
      frect(g, L, ey + 1, 2, 2, ink); frect(g, Rr, ey + 1, 2, 2, ink); break;
    case 'focus':
      frect(g, L - 1, ey - 1, 3, 1, ink); frect(g, Rr, ey - 1, 3, 1, ink);
      frect(g, L, ey, 2, 2, ink); frect(g, Rr, ey, 2, 2, ink); break;
  }
  const my = hcy + f.mo;
  switch (o.mouth) {
    case 'small': frect(g, hcx - 1, my, 2, 1, ink); break;
    case 'smile': px(g, hcx - 2, my - 1, ink); frect(g, hcx - 1, my, 2, 1, ink); px(g, hcx + 1, my - 1, ink); break;
    case 'open': frect(g, hcx - 1, my, 2, 2, ink); break;
  }
}
// Line art: outline of a shape given as an inside() test, interior cleared to black.
function lineShape(g, x0, y0, x1, y1, inside) {
  for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) {
    if (!inside(x, y)) continue;
    const edge = !(inside(x - 1, y) && inside(x + 1, y) && inside(x, y - 1) && inside(x, y + 1));
    px(g, x, y, edge ? 1 : 0);
  }
}
function inTri(p, a, b, c) {
  const cr = (p, q, r) => (p[0] - r[0]) * (q[1] - r[1]) - (q[0] - r[0]) * (p[1] - r[1]);
  const d1 = cr(p, a, b), d2 = cr(p, b, c), d3 = cr(p, c, a);
  return !((d1 < -0.5 || d2 < -0.5 || d3 < -0.5) && (d1 > 0.5 || d2 > 0.5 || d3 > 0.5));
}
function body(g, cx, from, gy, o, stem) {
  frect(g, cx - 1, from, 2, gy - from);
  frect(g, cx - 3, gy, 2, 1); frect(g, cx + 1, gy, 2, 1);
  const ay = from + 1 + Math.floor(stem / 3);
  const E = { down: [4, 3], up: [6, -4], out: [5, -1] };
  if (o.pad) { heldPad(g, cx, ay - 1, o.t); return; }
  leaf(g, cx - 2, ay, -E[o.arms[0]][0], E[o.arms[0]][1]);
  leaf(g, cx + 1, ay, E[o.arms[1]][0], E[o.arms[1]][1]);
}
// Tiny gamepad held in front of the stem; a button blinks as it is pressed.
function heldPad(g, cx, y, t) {
  frect(g, cx - 7, y - 1, 14, 7, 0);                         // clear around
  frect(g, cx - 5, y, 10, 1); frect(g, cx - 6, y + 1, 12, 3); frect(g, cx - 6, y + 4, 3, 1); frect(g, cx + 3, y + 4, 3, 1);
  frect(g, cx - 5, y + 2, 3, 1, 0); px(g, cx - 4, y + 1, 0); px(g, cx - 4, y + 3, 0);   // d-pad
  px(g, cx + 2, y + 2, t % 2 ? 1 : 0); px(g, cx + 3, y + 1, 0); px(g, cx + 4, y + 2, 0); px(g, cx + 3, y + 3, t % 2 ? 0 : 1);
  px(g, cx - 7, y + 2); px(g, cx + 6, y + 2);               // leaf-hands
}
function bandTails(g, x, y, t) { const f = t % 2; px(g, x, y); px(g, x + 1, y + f); px(g, x + 2, y + 1 + f); }

// Style 1 "Etoile": a lily seen from the front, five outlined pointed petals around a small face.
function lilyStar(g, o) {
  const R = 5, stem = Math.max(2, 7 - o.squash + o.stretch);
  const gy = o.ground - o.lift, cx = o.cx + o.bx, hcx = cx + o.hx, fc = hcx - 0.5;
  const hcy = gy - stem - R - 2;
  const deg = Math.PI / 180, droop = o.sleep ? 0.35 : 0;
  const base = [-90, -150, -30, 150, 30].map(a => a * deg);
  base.forEach((a, i) => {
    a += o.spin + (i === 0 ? 0 : (Math.cos(a) < 0 ? -droop : droop));
    const P = (ang, d) => [fc + Math.cos(ang) * d, hcy + Math.sin(ang) * d];
    tri(g, P(a, R + 9), P(a - 0.5, R), P(a + 0.5, R));
    tri(g, P(a, R + 6.5), P(a - 0.22, R + 1.8), P(a + 0.22, R + 1.8), 0);
  });
  disk(g, fc, hcy, R + 1, 0); disk(g, fc, hcy, R, 1);
  body(g, cx, hcy + R + 1, gy, o, stem);
  if (o.band) { frect(g, hcx - 5, hcy - 3, 10, 1, 0); bandTails(g, hcx + 5, hcy - 3, o.t); }
  face(g, hcx, hcy + 1, o, 0, { sp: 1, eh: 2, mo: 2 });
  return { hcx, hcy, top: hcy - R - 9 };
}

// Style 2 "Bouton": a closed bud in line art, white face on black, the tip sways.
function lilyBud(g, o) {
  const R = 6, stem = Math.max(2, 6 - o.squash + o.stretch);
  const gy = o.ground - o.lift, cx = o.cx + o.bx, hcx = cx + o.hx, fc = hcx - 0.5;
  const hcy = gy - stem - R - 1;
  const sway = o.sleep ? 5 : o.spin ? [-2, 0, 2, 0][o.t % 4] : o.hx * 2;
  const tip = [fc + sway, hcy - R - 9 + (o.sleep ? 3 : 0)];
  const a = [fc - R + 0.4, hcy - 2], b = [fc + R - 0.4, hcy - 2];
  const inside = (x, y) => ((x - fc) ** 2 + (y - hcy) ** 2 <= R * R + 0.6 * R) || (y <= hcy - 2 && inTri([x, y], tip, a, b));
  lineShape(g, hcx - R - 7, hcy - R - 10, hcx + R + 7, hcy + R + 1, inside);
  line(g, tip[0], tip[1] + 3, fc - 3, hcy - 4); line(g, tip[0], tip[1] + 3, fc + 3, hcy - 4);
  body(g, cx, hcy + R + 1, gy, o, stem);
  face(g, hcx, hcy + 2, o, 1, { sp: 2, eh: 2, mo: 2 });
  return { hcx, hcy, top: tip[1] };
}

// Style 3 "Chapeau": a round line-art creature wearing a lily bloom as a hat.
function lilyHat(g, o) {
  const R = 7, ry = R - o.squash * 0.5 + o.stretch * 0.5;
  const gy = o.ground - o.lift, cx = o.cx + o.bx, hcx = cx + o.hx, fc = hcx - 0.5;
  const by = gy - 1 - ry;
  const inside = (x, y) => ((x - fc) / R) ** 2 + ((y - by) / ry) ** 2 <= 1.08;
  lineShape(g, hcx - R - 2, Math.floor(by - ry - 2), hcx + R + 2, gy, inside);
  frect(g, cx - 4, gy, 3, 1); frect(g, cx + 1, gy, 3, 1);
  const A = { down: [2, 3], out: [3, 0], up: [2, -3] };
  const ax = R + 0.5, ay = by + 1;
  line(g, fc - ax, ay, fc - ax - A[o.arms[0]][0], ay + A[o.arms[0]][1]);
  line(g, fc + ax, ay, fc + ax + A[o.arms[1]][0], ay + A[o.arms[1]][1]);
  // the hat
  const top = by - ry + 1, deg = Math.PI / 180, droop = o.sleep ? 0.5 : 0;
  const P = (ang, d) => [fc + Math.cos(ang) * d, top + Math.sin(ang) * d];
  [-90, -145, -35].forEach((d0, i) => {
    const a = d0 * deg + o.spin + (i === 1 ? -droop : i === 2 ? droop : 0);
    tri(g, P(a, 8), P(a - 0.55, 1.5), P(a + 0.55, 1.5));
  });
  [-117, -63].forEach(d0 => { const a = d0 * deg + o.spin; line(g, ...P(a, 3), ...P(a, 8)); px(g, ...P(a, 9.2)); });
  if (o.band) { frect(g, hcx - 5, by - 3, 10, 1, 1); bandTails(g, hcx + 7, by - 3, o.t); }
  face(g, hcx, by + 1, o, 1, { sp: 2, eh: 3, mo: 3 });
  return { hcx, hcy: by, top: top - 9 };
}

// Style "Flamme": the bud silhouette, filled white, black face. The tip sways.
function budHead(g, o, R, hcy, fc) {
  const sway = o.sleep ? 5 : o.spin ? [-2, 0, 2, 0][o.t % 4] : (o.hx || 0) * 2;
  return { sway, tip: [fc + sway, hcy - R - 9 + (o.sleep ? 3 : 0)] };
}
function lilyFlame(g, o) {
  const R = 6, stem = Math.max(2, 6 - o.squash + o.stretch);
  const gy = o.ground - o.lift, cx = o.cx + o.bx, hcx = cx + o.hx, fc = hcx - 0.5;
  const hcy = gy - stem - R - 1;
  const { tip } = budHead(g, o, R, hcy, fc);
  const a = [fc - R + 0.4, hcy - 2], b = [fc + R - 0.4, hcy - 2];
  const inside = (x, y) => ((x - fc) ** 2 + (y - hcy) ** 2 <= R * R + 0.6 * R) || (y <= hcy - 2 && inTri([x, y], tip, a, b));
  for (let y = hcy - R - 10; y <= hcy + R + 1; y++) for (let x = hcx - R - 7; x <= hcx + R + 7; x++) if (inside(x, y)) px(g, x, y, 1);
  line(g, tip[0], tip[1] + 4, fc - 3, hcy - 4, 0); line(g, tip[0], tip[1] + 4, fc + 3, hcy - 4, 0);
  body(g, cx, hcy + R + 1, gy, o, stem);
  face(g, hcx, hcy + 2, o, 0, { sp: 2, eh: 2, mo: 2 });
  return { hcx, hcy, top: tip[1] };
}

// Style "Fleur": same round face, the top opens into a lily: three pointed tepals,
// the outer two curling out, two stamens peeking between them.
// Flower head: stamens, three tepals and the face disk, centred on (hcx - 0.5, hcy).
// tools/oled-mockup/gen_lily_sprites.js bakes it into sprites for the C code.
function flowerHead(g, hcx, hcy, sway, droop) {
  const R = 6, fc = hcx - 0.5, deg = Math.PI / 180;
  const cy = hcy - 3;                                   // petals grow from here
  const P = (ang, d) => [fc + Math.cos(ang) * d, cy + Math.sin(ang) * d];
  // stamens first, then tepals: pointed leaf shapes, each with a black rim so they
  // separate where they overlap, and a black midrib
  [-113, -67].forEach(d0 => { const a = d0 * deg + sway; line(g, ...P(a, 6), ...P(a, 14)); const e = P(a, 15); frect(g, e[0] - 0.5, e[1] - 0.5, 2, 2); });
  const tepal = (a, r0, r1, wd) => {
    const ux = Math.cos(a), uy = Math.sin(a), L = r1 - r0;
    const shape = (x, y, grow) => {
      const dx = x - fc, dy = y - cy, u = dx * ux + dy * uy - r0, v = -dx * uy + dy * ux;
      return u >= -grow && u <= L + grow && Math.abs(v) <= wd * Math.pow(Math.max(0, Math.sin(Math.PI * Math.min(1, Math.max(0, u / L)))), 0.7) + grow;
    };
    for (let y = Math.floor(cy - 20); y <= cy + 6; y++) for (let x = hcx - 16; x <= hcx + 16; x++) {
      if (shape(x, y, 0)) px(g, x, y, 1); else if (shape(x, y, 1)) px(g, x, y, 0);
    }
    line(g, ...[fc + ux * (r0 + 6), cy + uy * (r0 + 6)], ...[fc + ux * (r1 - 3), cy + uy * (r1 - 3)], 0);
  };
  tepal(-150 * deg + sway - droop, 2, 13, 3.2);
  tepal(-30 * deg + sway + droop, 2, 13, 3.2);
  tepal(-90 * deg + sway, 2, 15, 3.6);
  disk(g, fc, hcy, R + 1, 0); disk(g, fc, hcy, R, 1);
}
// The head poses the C code supports: index = FLOWER_SWAY order below.
const FLOWER_SWAYS = [[-0.25, 0], [-0.12, 0], [0, 0], [0.12, 0], [0.25, 0], [0.5, 0.45]];
function flowerSway(o) {
  if (o.sleep) return 5;
  if (o.spin) return [0, 2, 4, 2][o.t % 4];
  return 2 + (o.hx || 0);
}
function lilyFlower(g, o) {
  const R = 6, stem = Math.max(2, 6 - o.squash + o.stretch);
  const gy = o.ground - o.lift, cx = o.cx + o.bx, hcx = cx + o.hx;
  const hcy = gy - stem - R - 1;
  const [sway, droop] = FLOWER_SWAYS[flowerSway(o)];
  flowerHead(g, hcx, hcy, sway, droop);
  body(g, cx, hcy + R + 1, gy, o, stem);
  face(g, hcx, hcy + 1, o, 0, { sp: 2, eh: 2, mo: 2 });
  return { hcx, hcy, top: hcy - 16 };
}
const STYLES = { v1: lily, star: lilyStar, bud: lilyBud, hat: lilyHat, flame: lilyFlame, flower: lilyFlower };

// ===== The three variants =====
function drawA(g, s) { clear(g); hud(g, s.layer); iconA(g, s.layer, s.t); stack(g, s.layer, s.t); gauge(g, s.wpm); }
function drawC(g, s) {
  clear(g); hud(g, s.layer);
  clip(g, 14, 61); drawLily(g, s.layer, s.t, s.wpm, s.sleeping, 16, 58, 14, s.style); clip(g, 0, H);
  stack(g, s.layer, s.t); gauge(g, s.wpm);
}
function drawB(g, s) {
  clear(g); hud(g, s.layer);
  clip(g, 14, 86); drawLily(g, s.layer, s.t, s.wpm, s.sleeping, 16, 80, 14, s.style);
  for (let x = 1; x < 31; x += 2) px(g, x, 82);
  clip(g, 0, H);
  text(g, 'WPM ' + Math.round(s.wpm), 89);
  const hist = s.hist;
  for (let i = 0; i < hist.length; i++) {
    const hgt = Math.max(1, Math.round(26 * Math.min(hist[i], 150) / 150));
    frect(g, 2 + i, 125 - hgt, 1, hgt);
  }
  for (let x = 2; x < 30; x += 2) px(g, x, 126);
}

if (typeof module !== 'undefined') module.exports = { W, H, makeScreen, drawA, drawB, drawC, LAYERS, flowerHead, FLOWER_SWAYS };
