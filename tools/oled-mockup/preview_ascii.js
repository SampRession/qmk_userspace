const c = require(process.argv[2]);
const v = process.argv[3], y0 = +process.argv[4], y1 = +process.argv[5], t = +(process.argv[6]||1);
const sleeping = process.argv[7] === 's';
const layers = (process.argv[8] || c.LAYERS.join(',')).split(',');
const shots = layers.map(l => { const g = c.makeScreen(); c['draw' + v](g, { layer: l, t, wpm: 40, sleeping, hist: Array(28).fill(40) }); return { l, g }; });
console.log(shots.map(s => s.l.padEnd(32)).join('  '));
for (let y = y0; y < y1; y++) console.log(shots.map(s => { let r = ''; for (let x = 0; x < 32; x++) r += s.g.buf[y * 32 + x] ? '#' : '.'; return r; }).join('  '));
