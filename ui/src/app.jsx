/* ============================================================
   Jove · analog-inspired polysynth — UI
   ============================================================ */
const { useState, useEffect, useRef, useMemo } = React;
const B = window.JoveBridge;

/* ---- enum label lists (index = engine contract, see SynthParams.h) ---- */
const VOICE_MODES = ["POLY", "MONO", "UNISON"];
const GLIDE_MODES = ["OFF", "ON", "LEGATO"];
const FOOTAGE     = ["32'", "16'", "8'", "4'", "2'"];
const SYNC_MODES  = ["OFF", "SOFT", "HARD"];
const FILTER_MODES= ["MOOG", "LP", "HP", "BP", "NOTCH", "LPG", "STEINER"];
const ARP_MODES   =["UP","DOWN","UP-DN","UP-DN+","DN-UP","PINGPONG","CONV","DIV","CON-DIV","ASPLAYED","RANDOM","CHORD"];
const DIVISIONS   = ["1/64","1/32T","1/32","1/16T","1/16","1/8T","1/16.","1/8","1/4T","1/8.","1/4","1/4.","1/2"];
const MOD_SRC     = ["OFF","LFO1","LFO2","LFO3","AMP EG","FLT EG","AUX EG","VEL","KEY","MWHEEL","ATOUCH","BEND","RAND","NOTE","MPE PRS","MPE TMB","MPE BND","SEQ1","SEQ2","SEQ3","SEQ4"];
const SEQ_CURVES  = ["STEP","LIN","SMOOTH"];
const SEQ_DIRS    = ["FWD","REV","PEND","RND"];
const SEQ_MODES   = ["CURVE","MELODIC"];
const MOD_DST     = ["OFF","PITCH","O2 PIT","O3 PIT","MORPH1","MORPH2","MORPH3","PW1","PW2","PW3","OSCMIX","SUB","NOISE","CUTOFF","RESO","FDRIVE","AMP","PAN","L1 RATE","L2 RATE","L3 RATE","L1 DPT","L2 DPT","L3 DPT","FXSEND","FXPARM","FM","RING","ENVFLT","DETUNE"];
const CHORUS_MODES= ["Chorus I", "Chorus II", "Ensemble", "Combine"];
const QUALITY     = ["Eco", "HQ", "Ultra"];
const CATEGORIES  = ["PAD","LEAD","BASS","ARP","STAB","KEYS","PLUCK","FX","DRONE","PERC","AMB","SEQ"];

/* ModDest index -> the knob param id it drives (index-aligned to MOD_DST). null
   for destinations without a single knob (e.g. global PITCH). Used to paint a
   live modulation arc + dot on the affected knob, Doobie-style. */
const DEST_PARAM = [
  null, null, "osc2Detune", "osc3Detune", "osc1Morph", "osc2Morph", "osc3Morph",
  "osc1Pw", "osc2Pw", "osc3Pw", "oscMix", "subLevel", "noiseLevel", "cutoff",
  "resonance", "filterDrive", "ampGain", "pan", "lfo1Rate", "lfo2Rate", "lfo3Rate",
  "lfo1Depth", "lfo2Depth", "lfo3Depth", null, null, "fm2to1", "ringMod",
  "envFilterAmt", null];
const MOD_SCALE = 0.25; // visual: mod amount 1.0 -> a quarter of the knob's travel

const ModContext = React.createContext({ map: {}, lfo: [0, 0, 0], src: [] });

/* Read ALL 32 mod slots and build paramId -> [{src, amt}] for the indicators
   and the live oscilloscope mod. (Was 10 — any route in a higher slot, which is
   where preset routes commonly land, never reached the knob arcs or the osc
   scope, so waveforms looked static under modulation.) */
function useModMap() {
  const slots = [];
  for (let i = 1; i <= 32; i++) {
    const [src] = B.useChoice("mod" + i + "Src");
    const [dst] = B.useChoice("mod" + i + "Dst");
    const [amtN] = B.useSlider("mod" + i + "Amt"); // 0..1 of the -2..+2 range
    slots.push({ src, dst, amt: amtN * 4 - 2 });
  }
  const map = {};
  slots.forEach((s) => {
    if (s.src <= 0 || Math.abs(s.amt) < 0.001) return;
    const pid = DEST_PARAM[s.dst];
    if (!pid) return;
    (map[pid] = map[pid] || []).push({ src: s.src, amt: s.amt });
  });
  return map;
}

/* ============================ atoms ============================ */
function fmt(v) {
  const a = Math.abs(v);
  if (a >= 100) return v.toFixed(0);
  if (a >= 10)  return v.toFixed(1);
  return v.toFixed(2);
}

function Knob({ id, label, bipolar = false, big = false, small = false }) {
  const [v, set, scaled] = B.useSlider(id);
  const ref = useRef(null);
  const drag = useRef(null);
  const lastClick = useRef(0);
  const A0 = -135, SWEEP = 270;
  const D = big ? 58 : (small ? 40 : 48), c = D / 2, sw = big ? 4 : 3.2, R = c - sw - 1;
  const ang = A0 + v * SWEEP;
  const rad = (d) => d * Math.PI / 180;
  const pt = (r, a) => [c + r * Math.sin(rad(a)), c - r * Math.cos(rad(a))];
  const [px, py] = pt(R, ang);
  const arc = (a0, a1) => {
    const [x0, y0] = pt(R, a0), [x1, y1] = pt(R, a1);
    return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A ${R} ${R} 0 ${(a1 - a0) > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  };
  const fillFrom = bipolar ? A0 + 0.5 * SWEEP : A0;
  const down = (e) => {
    e.preventDefault();
    // manual double-click reset (mousedown preventDefault suppresses dblclick):
    // bipolar knobs reset to centre (0), unipolar to 0.
    const now = e.timeStamp || 0;
    if (now - lastClick.current < 330) { lastClick.current = 0; set(bipolar ? 0.5 : 0); return; }
    lastClick.current = now;
    const p = e.touches ? e.touches[0] : e;
    drag.current = { y: p.clientY, v };
    const move = (ev) => {
      const q = ev.touches ? ev.touches[0] : ev;
      const fine = ev.shiftKey ? 0.25 : 1;
      set(drag.current.v + (drag.current.y - q.clientY) / 220 * fine);
    };
    const up = () => {
      window.removeEventListener('mousemove', move); window.removeEventListener('mouseup', up);
      window.removeEventListener('touchmove', move); window.removeEventListener('touchend', up);
    };
    window.addEventListener('mousemove', move); window.addEventListener('mouseup', up);
    window.addEventListener('touchmove', move, { passive: false }); window.addEventListener('touchend', up);
  };
  const wheel = (e) => { e.preventDefault(); set(v + (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? 0.01 : 0.03)); };
  let val = v; try { val = scaled(); } catch (_) {}

  // modulation indicators (range band + live moving dot), Doobie-style
  const mc = React.useContext(ModContext);
  const minfo = id && mc.map[id];
  let mrange = 0, mlive = 0;
  if (minfo) {
    // live offset driven by EVERY routed source's current value (LFOs,
    // envelopes, wheel, velocity, ...), so the dot moves at the destination
    // for any active modulation.
    minfo.forEach((m) => {
      mrange += Math.abs(m.amt);
      mlive += ((mc.src && mc.src[m.src]) || 0) * m.amt;
    });
    mrange = Math.min(0.5, mrange * MOD_SCALE);
    mlive = Math.max(-0.6, Math.min(0.6, mlive * MOD_SCALE));
  }
  const modA0 = A0 + Math.max(0, v - mrange) * SWEEP;
  const modA1 = A0 + Math.min(1, v + mrange) * SWEEP;
  const liveAng = A0 + Math.max(0, Math.min(1, v + mlive)) * SWEEP;
  const [mx, my] = pt(R, liveAng);

  return (
    <div className={"knob" + (big ? " big" : "") + (minfo ? " modded" : "")} onMouseDown={down}
         onTouchStart={down} onWheel={wheel} onDoubleClick={() => set(bipolar ? 0.5 : 0)} title={label}>
      <svg width={D} height={D} viewBox={`0 0 ${D} ${D}`}>
        <path d={arc(A0, A0 + SWEEP)} className="k-track" strokeWidth={sw} />
        {minfo && <path d={arc(modA0, modA1)} className="k-mod" strokeWidth={sw + 2.5} />}
        <path d={arc(Math.min(fillFrom, ang), Math.max(fillFrom, ang))} className="k-fill" strokeWidth={sw} />
        <circle cx={c} cy={c} r={R - sw - 1.5} className="k-body" />
        <line x1={c} y1={c} x2={px} y2={py} className="k-ptr" />
        {minfo && <circle cx={mx} cy={my} r={big ? 3.4 : 3} className="k-moddot" />}
      </svg>
      <div className="k-lab">{label}</div>
      <div className="k-val">{fmt(val)}</div>
    </div>
  );
}

function Switch({ id, label }) {
  const [v, set] = B.useToggle(id);
  return (
    <button className={"chip" + (v ? " on" : "")} onClick={() => set(!v)}>
      <span className="led" />{label}
    </button>
  );
}

// compact master-FX enable button for the top bar (lit = on, dim = bypassed)
function FxBtn({ id, label }) {
  const [v, set] = B.useToggle(id);
  return (
    <button className={"fxbtn" + (v ? " on" : "")} onClick={() => set(!v)} title={label + (v ? " on" : " bypassed")}>
      {label}
    </button>
  );
}

function Seg({ id, options, label }) {
  const [idx, set] = B.useChoice(id);
  return (
    <div className="seg-wrap">
      {label && <div className="cl">{label}</div>}
      <div className="seg">
        {options.map((o, i) =>
          <button key={i} className={i === idx ? "on" : ""} onClick={() => set(i)}>{o}</button>)}
      </div>
    </div>
  );
}

// Custom dropdown (no native <select>): flat themed, and crucially it never
// holds keyboard focus, so typing / computer-MIDI never changes the value.
function Dropdown({ idx, options, onPick }) {
  const [open, setOpen] = useState(false);
  const ref = useRef(null);
  useEffect(() => {
    if (!open) return;
    const close = (e) => { if (ref.current && !ref.current.contains(e.target)) setOpen(false); };
    window.addEventListener("mousedown", close);
    return () => window.removeEventListener("mousedown", close);
  }, [open]);
  return (
    <div className={"dd" + (open ? " open" : "")} ref={ref}>
      <button className="dd-head" tabIndex={-1}
              onMouseDown={(e) => { e.preventDefault(); setOpen((o) => !o); }}>
        <span className="dd-cur">{options[idx]}</span><span className="dd-caret" />
      </button>
      {open &&
        <div className="dd-list">
          {options.map((o, i) =>
            <button key={i} tabIndex={-1} className={"dd-opt" + (i === idx ? " on" : "")}
                    onMouseDown={(e) => { e.preventDefault(); onPick(i); setOpen(false); }}>{o}</button>)}
        </div>}
    </div>
  );
}

function Sel({ id, options, label }) {
  const [idx, set] = B.useChoice(id);
  return (
    <div className="sel-wrap">
      {label && <div className="cl">{label}</div>}
      <Dropdown idx={idx} options={options} onPick={set} />
    </div>
  );
}

/* Integer params (AudioParameterInt) are backed by a SLIDER relay, not a combo —
   so they need slider-based controls, not <Seg>/<Sel> (which use combo relays). */
function IntSeg({ id, label, min, max }) {
  const [v, set] = B.useSlider(id);
  const cur = Math.round(min + v * (max - min));
  const opts = []; for (let i = min; i <= max; i++) opts.push(i);
  return (
    <div className="seg-wrap">
      {label && <div className="cl">{label}</div>}
      <div className="seg">
        {opts.map((i) =>
          <button key={i} className={i === cur ? "on" : ""}
                  onClick={() => set((i - min) / (max - min))}>{i}</button>)}
      </div>
    </div>
  );
}

function IntPick({ id, label, min, max }) {
  const [v, set] = B.useSlider(id);
  const cur = Math.round(min + v * (max - min));
  const opts = []; for (let i = min; i <= max; i++) opts.push("" + i);
  return (
    <div className="sel-wrap">
      {label && <div className="cl">{label}</div>}
      <Dropdown idx={cur - min} options={opts} onPick={(i) => set(i / (max - min))} />
    </div>
  );
}

function Panel({ title, children }) {
  return (
    <section className="panel">
      <header className="ph">{title}</header>
      <div className="pbody">{children}</div>
    </section>
  );
}

/* Fixed pseudo-random levels (-1..1) for sample & hold — deterministic so the
   icon and scope draw clean, recognisable stepped art instead of spiky noise. */
const SNH_LEVELS = [0.55, -0.35, 0.85, -0.7, 0.2, -0.55, 0.45, -0.15];

/* Stairstep point list for S&H: horizontal holds joined by vertical jumps. */
function stairPoints(steps, W, mid, a) {
  const pts = [];
  for (let i = 0; i < steps; i++) {
    const y = (mid - SNH_LEVELS[i % SNH_LEVELS.length] * a).toFixed(1);
    pts.push((i / steps * W).toFixed(1) + "," + y, ((i + 1) / steps * W).toFixed(1) + "," + y);
  }
  return pts.join(" ");
}

/* SVG waveform icon. Type: 0 sine, 1 tri, 2 saw-up, 3 saw-dn, 4 square, 5 pulse,
   6 sample&hold. Drawn as ~1.6 cycles. */
function wavePoints(type, pw, W, H) {
  const mid = H / 2, a = H / 2 - 2.5, N = 60, cyc = 1.6;
  if (type === 6) return stairPoints(6, W, mid, a); // clean staircase, not noise
  const shape = (t) => {
    t = (t % 1 + 1) % 1;
    switch (type) {
      case 0: return Math.sin(2 * Math.PI * t);
      case 1: return t < 0.5 ? 4 * t - 1 : 3 - 4 * t;
      case 2: return 2 * t - 1;
      case 3: return 1 - 2 * t;
      case 4: return t < 0.5 ? 1 : -1;
      case 5: return t < (pw || 0.3) ? 1 : -1;
      default: return 0;
    }
  };
  const pts = [];
  for (let i = 0; i <= N; i++) { const t = (i / N) * cyc; pts.push(((i / N) * W).toFixed(1) + "," + (mid - shape(t) * a).toFixed(1)); }
  return pts.join(" ");
}
function WaveIcon({ w, pw }) {
  const W = 28, H = 15;
  return <svg className="wico" width={W} height={H} viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
    <polyline points={wavePoints(w, pw, W, H)} /></svg>;
}

/* Oscillator waveform selector (icons). Sets morph + pulse width to named
   shapes; the MORPH knob still allows continuous in-between morphing. */
const OSC_WAVES = [
  { ico: 0, m: 0.0, pw: 0.5 }, { ico: 1, m: 0.25, pw: 0.5 }, { ico: 2, m: 0.5, pw: 0.5 },
  { ico: 4, m: 1.0, pw: 0.5 }, { ico: 5, m: 1.0, pw: 0.3 }, { ico: 5, m: 1.0, pw: 0.15 },
];
function WaveSelect({ morphId, pwId }) {
  const [m, setM] = B.useSlider(morphId);
  const [pw, setPw] = B.useSlider(pwId);
  let best = 0, bd = 9;
  OSC_WAVES.forEach((w, i) => {
    const d = Math.abs(m - w.m) + (w.m >= 0.999 ? Math.abs(pw - w.pw) : 0);
    if (d < bd) { bd = d; best = i; }
  });
  return (
    <div className="waveico">
      {OSC_WAVES.map((w, i) =>
        <button key={i} tabIndex={-1} className={"wbtn" + (i === best ? " on" : "")}
                onMouseDown={(e) => { e.preventDefault(); setM(w.m); setPw(w.pw); }}>
          <WaveIcon w={w.ico} pw={w.pw} />
        </button>)}
    </div>
  );
}

/* LFO waveform selector (icons), driving the choice param. Index order matches
   LFO_WAVES: SINE, TRI, SAW+, SAW-, SQR, S&H. */
const LFO_ICO = [0, 1, 2, 3, 4, 6];
function LfoWaveSelect({ id }) {
  const [idx, set] = B.useChoice(id);
  return (
    <div className="waveico">
      {LFO_ICO.map((ic, i) =>
        <button key={i} tabIndex={-1} className={"wbtn" + (i === idx ? " on" : "")}
                onMouseDown={(e) => { e.preventDefault(); set(i); }}>
          <WaveIcon w={ic} />
        </button>)}
    </div>
  );
}

/* ============================ visualizers ============================ */
/* Live oscillator shape from morph + pulse width (sine->tri->saw->pulse),
   so each oscillator card shows what it actually produces. */
function oscShape(m, pw, t) {
  t = t - Math.floor(t);
  const sine = Math.sin(2 * Math.PI * t);
  const tri = t < 0.5 ? 4 * t - 1 : 3 - 4 * t;
  const saw = 2 * t - 1;
  const pulse = t < (pw || 0.3) ? 1 : -1;
  const lerp = (a, b, x) => a + (b - a) * x;
  if (m < 0.25) return lerp(sine, tri, m / 0.25);
  if (m < 0.5) return lerp(tri, saw, (m - 0.25) / 0.25);
  return lerp(saw, pulse, (m - 0.5) / 0.5);
}
/* JS mirror of the engine wavetable bank (Wavetables.h) so the scope shows the
   real table; table index + morph blend toward the next table. */
/* names: 16 hand-tuned character tables + 48 parametric AKWF-style families.
   The families mirror wtExtraDesc/wtExtraHarmonic in engine/Wavetables.h exactly. */
const WT_NAMED = ["SINE", "TRIANGLE", "SAW", "SQUARE", "PULSE 25", "PULSE 12", "BRIGHT", "ORGAN",
  "ODD", "FORMANT A", "FORMANT B", "SPARSE", "HOLLOW", "BRASS", "STRING", "VOCAL"];
function wtExtraName(e) {
  if (e < 12) return "SAW " + (e + 1);
  if (e < 24) return "FORMANT " + (e - 11);
  if (e < 36) return "VOWEL " + (e - 23);
  if (e < 42) return "ODD " + (e - 35);
  return "COMB " + (2 + (e - 42));
}
const WT_NAMES = WT_NAMED.concat(Array.from({ length: 48 }, (_, e) => wtExtraName(e)));
function wtExtraDesc(e) {
  if (e < 12) return { type: 0, p1: 0.55 + e * 0.26, p2: 0 };
  if (e < 24) return { type: 2, p1: 2 + (e - 12) * 2, p2: 2.5 };
  if (e < 36) { const k = e - 24; return { type: 3, p1: 2 + k, p2: 7 + k * 1.3 }; }
  if (e < 42) return { type: 5, p1: 0.9 + (e - 36) * 0.32, p2: 0 };
  return { type: 4, p1: 2 + (e - 42), p2: 0 };
}
function wtExtraHarmonic(d, n) {
  const fn = n;
  switch (d.type) {
    case 0: return 1 / Math.pow(fn, d.p1);
    case 2: { const c = d.p1, s = d.p2; return Math.exp(-((fn - c) * (fn - c)) / (2 * s * s)) + 0.06 / fn; }
    case 3: { const a = Math.exp(-((fn - d.p1) * (fn - d.p1)) / 8) + 0.6 * Math.exp(-((fn - d.p2) * (fn - d.p2)) / 12); return a / fn + 0.04 / fn; }
    case 4: { const k = d.p1 | 0; return (n % k === 1) ? 1 / fn : 0; }
    case 5: return (n % 2 === 1) ? 1 / Math.pow(fn, d.p1) : 0;
    default: return 1 / fn;
  }
}
function wtHarmonic(w, n) {
  if (w >= 16) return wtExtraHarmonic(wtExtraDesc(w - 16), n);
  const fn = n;
  switch (w) {
    case 0: return n === 1 ? 1 : 0;
    case 1: return (n % 2 === 1) ? (1 / (fn * fn)) * ((n % 4 === 1) ? 1 : -1) : 0;
    case 2: return 1 / fn;
    case 3: return (n % 2 === 1) ? 1 / fn : 0;
    case 4: return Math.abs(Math.sin(fn * Math.PI * 0.25)) / fn * 2;
    case 5: return Math.abs(Math.sin(fn * Math.PI * 0.125)) / fn * 2;
    case 6: return 1 / Math.sqrt(fn);
    case 7: { const d = [1, 0.7, 0.5, 0.4, 0, 0.3, 0, 0.2]; return n <= 8 ? d[n - 1] : 0; }
    case 8: return (n % 2 === 1) ? 1 / (fn * fn) : 0;
    case 9: { const c = 3; return Math.exp(-((fn - c) * (fn - c)) / 4) + 0.12 / fn; }
    case 10: { const c = 9; return Math.exp(-((fn - c) * (fn - c)) / 12) + 0.06 / fn; }
    case 11: return (n === 1 || n === 3 || n === 7 || n === 11 || n === 17) ? 1 / fn : 0;
    case 12: return (n % 2 === 1) ? 1.2 / fn : 0;
    case 13: { const c = 5; return (1 / fn) * (0.5 + Math.exp(-((fn - c) * (fn - c)) / 20)); }
    case 14: return 1 / Math.pow(fn, 1.2);
    case 15: { const a = Math.exp(-((fn - 4) * (fn - 4)) / 3) + 0.6 * Math.exp(-((fn - 9) * (fn - 9)) / 6) + 0.3 * Math.exp(-((fn - 13) * (fn - 13)) / 8); return a / fn + 0.05 / fn; }
    default: return 1 / fn;
  }
}
function wtSample(w, t) { let s = 0; for (let nn = 1; nn <= 48; nn++) { const a = wtHarmonic(w, nn); if (Math.abs(a) > 1e-5) s += a * Math.sin(2 * Math.PI * nn * t); } return s; }

/* table-name dropdown driven by the integer wtTable slider param */
function WtTableSel({ id }) {
  const [v, set] = B.useSlider(id);
  const N = WT_NAMES.length, idx = Math.round(v * (N - 1));
  return (
    <div className="sel-wrap">
      <div className="cl">WAVETABLE</div>
      <Dropdown idx={idx} options={WT_NAMES} onPick={(i) => set(i / (N - 1))} />
    </div>
  );
}

function OscWave({ morphId, pwId, crushId, on, wt, tableId, wtMorphId }) {
  const m0 = B.useSlider(morphId)[0];
  const pw0 = B.useSlider(pwId)[0];
  const crush = B.useSlider(crushId)[0];
  const wtV = B.useSlider(tableId)[0];
  const wtMorphV = B.useSlider(wtMorphId)[0];
  // live modulation reaching this oscillator's morph / pulse-width (matrix routes
  // to MORPHn / PWn), so the scope visibly moves as LFOs/envelopes modulate it.
  const mc = React.useContext(ModContext);
  let mAdd = 0, pwAdd = 0;
  ((mc.map && mc.map[morphId]) || []).forEach((x) => { mAdd += ((mc.src && mc.src[x.src]) || 0) * x.amt; });
  ((mc.map && mc.map[pwId]) || []).forEach((x) => { pwAdd += ((mc.src && mc.src[x.src]) || 0) * x.amt * 0.5; });
  const m = Math.max(0, Math.min(1, m0 + mAdd));
  const pw = Math.max(0.02, Math.min(0.98, pw0 + pwAdd));
  const levels = crush > 0.01 ? Math.pow(2, 8.5 - crush * 8.0) : 0; // mirror engine bit-crush
  const W = 160, H = 54, cy = H / 2, amp = H / 2 - 4, N = 160, cyc = 2;
  // wavetable scan (mirrors Voice.h): base table + (wtMorph knob + live MORPH
  // mod) sweeps the WHOLE bank; blend the two bracketing tables by the fraction.
  const N1 = WT_NAMES.length - 1;
  let wi0 = 0, wi1 = 0, frac = 0, wtNorm = 1;
  if (wt) {
    const base = wtV * N1;
    let tp = base + (wtMorphV + mAdd) * (N1 - base); // span = tables above the base
    tp = Math.max(0, Math.min(N1, tp));
    wi0 = Math.floor(tp); wi1 = Math.min(N1, wi0 + 1); frac = tp - wi0;
    let pk = 1e-6;
    for (let i = 0; i < 64; i++) { const s = wtSample(wi0, i / 64) * (1 - frac) + wtSample(wi1, i / 64) * frac; pk = Math.max(pk, Math.abs(s)); }
    wtNorm = 1 / pk;
  }
  const pts = [];
  for (let i = 0; i <= N; i++) {
    const t = (i / N) * cyc, tt = t - Math.floor(t);
    let y = wt ? (wtSample(wi0, tt) * (1 - frac) + wtSample(wi1, tt) * frac) * wtNorm
               : oscShape(m, pw, t);
    if (levels) y = Math.round(y * levels) / levels;
    pts.push(((i / N) * W).toFixed(1) + "," + (cy - y * amp).toFixed(1));
  }
  const grid = [0.25, 0.5, 0.75].map((f) => (f * W).toFixed(1));
  return (
    <svg className={"viz oscwave" + (on ? "" : " off")} viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      {grid.map((x, i) => <line key={i} x1={x} y1="0" x2={x} y2={H} className="grid" />)}
      <line x1="0" y1={cy} x2={W} y2={cy} className="mid" />
      <polyline points={pts.join(" ")} />
    </svg>
  );
}

function EnvViz({ n }) {
  // Use REAL engineering values, not the skewed 0..1 normals: A/D/R in seconds,
  // S as a 0..1 level. Segment widths are sqrt-compressed seconds so a long
  // attack doesn't swamp the view but proportions stay truthful.
  const a = B.useSlider("env" + n + "Attack")[2]();
  const d = B.useSlider("env" + n + "Decay")[2]();
  const s = B.useSlider("env" + n + "Sustain")[0];
  const r = B.useSlider("env" + n + "Release")[2]();
  const W = 140, H = 40, pad = 3;
  const cw = (t) => Math.sqrt(Math.max(0, t));
  const aw = 0.12 + cw(a), dw = 0.12 + cw(d), sw = 0.85, rw = 0.12 + cw(r);
  const tot = aw + dw + sw + rw, uw = (W - 2 * pad) / tot;
  const sy = pad + (H - 2 * pad) * (1 - s);
  let x = pad;
  const pts = [[x, H - pad]];
  x += aw * uw; pts.push([x, pad]);
  x += dw * uw; pts.push([x, sy]);
  x += sw * uw; pts.push([x, sy]);
  x += rw * uw; pts.push([x, H - pad]);
  // live output level (engine publishes each env as a mod source: amp=4, flt=5,
  // aux=6) -> a glowing level line that rides up/down as the note plays.
  const mc = React.useContext(ModContext);
  const lvl = Math.max(0, Math.min(1, (mc.src && mc.src[n + 3]) || 0));
  const ly = pad + (H - 2 * pad) * (1 - lvl);
  return (
    <svg className="viz env" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <polyline points={pts.map((p) => p[0].toFixed(1) + "," + p[1].toFixed(1)).join(" ")} />
      <polygon className="fillz" points={`${pad},${H - pad} ${pts.map((p) => p[0].toFixed(1) + "," + p[1].toFixed(1)).join(" ")} ${(x).toFixed(1)},${H - pad}`} />
      {lvl > 0.01 && <line x1={pad} y1={ly.toFixed(1)} x2={W - pad} y2={ly.toFixed(1)} className="env-live" />}
      {lvl > 0.01 && <circle cx={W - pad - 1} cy={ly.toFixed(1)} r="2.4" className="env-dot" />}
    </svg>
  );
}

// Draw the LFO's actual waveform shape (two cycles) from its wave type — exact at
// any rate, unlike a 30 Hz rolling scope which aliases. A live playhead dot rides
// the curve at the engine-reported phase so motion is still visible and correct.
function LfoScope({ n }) {
  const [wave] = B.useChoice("lfo" + n + "Wave");
  // OFFSET shifts the whole wave (the engine adds it as a DC bias); shown but
  // bounded so the trace never clips. DEPTH is its own knob, not drawn as scale.
  const offset = Math.max(-0.45, Math.min(0.45, B.useSlider("lfo" + n + "Offset")[2]() * 0.45));
  const meters = B.useEvent("meters", { lfoPhase: [0, 0, 0] });
  const ph = (meters.lfoPhase && meters.lfoPhase[n - 1]) || 0;
  const W = 140, H = 26, cy = H / 2, amp = (H / 2 - 2.5) * 0.55, mid = cy - offset * (H / 2 - 2.5), cycles = 2;
  const shape = (t) => {
    t = t - Math.floor(t);
    switch (wave) {
      case 0: return Math.sin(2 * Math.PI * t);
      case 1: return t < 0.5 ? 4 * t - 1 : 3 - 4 * t;
      case 2: return 2 * t - 1;
      case 3: return 1 - 2 * t;
      case 4: return t < 0.5 ? 1 : -1;
      default: return 0;
    }
  };
  const phPos = ((ph * cycles) % cycles) / cycles; // 0..1 across the view
  const phx = phPos * W;
  let pointStr, phy;
  if (wave === 5) {                       // sample & hold: clean staircase
    const steps = 4 * cycles, sp = [];
    for (let i = 0; i < steps; i++) {
      const yy = (mid - SNH_LEVELS[i % SNH_LEVELS.length] * amp).toFixed(1);
      sp.push((i / steps * W).toFixed(1) + "," + yy, ((i + 1) / steps * W).toFixed(1) + "," + yy);
    }
    pointStr = sp.join(" ");
    const si = Math.min(steps - 1, Math.floor(phPos * steps));
    phy = mid - SNH_LEVELS[si % SNH_LEVELS.length] * amp;
  } else {
    const N = 96, pts = [];
    for (let i = 0; i <= N; i++) {
      const t = (i / N) * cycles;
      pts.push(((i / N) * W).toFixed(1) + "," + (mid - shape(t) * amp).toFixed(1));
    }
    pointStr = pts.join(" ");
    phy = mid - shape(ph * cycles) * amp;
  }
  return (
    <svg className="viz scope" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <line x1="0" y1={cy} x2={W} y2={cy} className="mid" />
      <polyline points={pointStr} />
      <circle cx={phx.toFixed(1)} cy={phy.toFixed(1)} r="2" className="scope-dot" />
    </svg>
  );
}

// Accurate-ish filter response: the standard 2-pole (biquad) magnitude, drawn in
// log-frequency / dB. MOOG (4-pole ladder) is approximated by squaring. Shows a
// flat-then-rolloff LP, rising HP, a BP peak, a notch dip — with a real resonance
// bump at the cutoff that grows with Resonance.
function FilterCurve() {
  const [cut] = B.useSlider("cutoff");
  const [res] = B.useSlider("resonance");
  const [efa] = B.useSlider("envFilterAmt");
  const [mode] = B.useChoice("filterMode");
  // live modulation reaching the filter: cutoff in octaves, resonance linear.
  // Matrix routes use the same data the knobs paint with; the dedicated
  // filter-env -> cutoff path adds envFilterAmt * (live FLT EG) * 6 octaves.
  const mc = React.useContext(ModContext);
  // only the cutoff sweeps live (matrix + filter env); resonance is left static
  // so the peak height stays steady instead of wiggling with every meter tick.
  let cutOct = 0;
  (mc.map && mc.map["cutoff"] || []).forEach((m) => { cutOct += ((mc.src && mc.src[m.src]) || 0) * m.amt * 4; });
  cutOct += efa * ((mc.src && mc.src[5]) || 0) * 6; // MOD_SRC[5] = FLT EG
  const W = 180, H = 38, N = 96;
  const fmin = Math.log2(20), fmax = Math.log2(20000);
  const fc = Math.max(20, Math.min(20000, 20 * Math.pow(2, cut * 9.6 + cutOct)));
  const resL = Math.max(0, Math.min(1, res));
  const ladder = mode === 0, isLpg = mode === 5, isSteiner = mode === 6;
  // exact engine mappings: SVF damping k = 2 - 1.9*res (Steiner is peakier);
  // ladder feedback k = 4*res with the engine's (1 + 0.5k) output makeup.
  const svfRes = isSteiner ? (0.2 + resL * 0.78) : (resL * 0.97);
  const kSvf = 2 - 1.9 * Math.min(0.95, svfRes);
  const kLad = 4 * resL;
  // LPG amplitude gate tracks cutoff (matches engine gate_)
  let gate = 1;
  if (isLpg) { let nrm = Math.log2(fc / 20) / Math.log2(20000 / 20); nrm = Math.max(0, Math.min(1, nrm)); gate = 0.12 + 0.88 * nrm * nrm; }
  const mag = (f) => {
    if (ladder) {                       // 4 one-pole stages + feedback (Stilson-Smith)
      const wn = f / fc;
      const gm = Math.pow(1 / Math.sqrt(1 + wn * wn), 4), gp = -4 * Math.atan(wn);
      const gre = gm * Math.cos(gp), gim = gm * Math.sin(gp);
      const dre = 1 + kLad * gre, dim = kLad * gim, dd = dre * dre + dim * dim;
      const hre = (gre * dre + gim * dim) / dd, him = (gim * dre - gre * dim) / dd;
      return Math.sqrt(hre * hre + him * him) * (1 + 0.5 * kLad);
    }
    const w = f / fc, w2 = w * w;        // 2-pole SVF family (TPT): denom |1-w² + j w k|
    const denom = Math.sqrt((1 - w2) * (1 - w2) + (w * kSvf) * (w * kSvf)) || 1e-9;
    let h;
    if (mode === 2) h = w2 / denom;                   // HP
    else if (mode === 3) h = (w * kSvf) / denom;      // BP (unity at resonance)
    else if (mode === 4) h = Math.abs(1 - w2) / denom; // notch
    else h = 1 / denom;                               // LP, LPG, Steiner
    return h * gate;
  };
  const pts = [];
  for (let i = 0; i <= N; i++) {
    const f = Math.pow(2, fmin + (i / N) * (fmax - fmin));
    const db = 20 * Math.log10(Math.max(1e-4, mag(f)));
    const y = Math.max(0, Math.min(1, (db + 48) / 72)); // -48..+24 dB, peaks never clip
    pts.push(((i / N) * W).toFixed(1) + "," + (H - 2 - y * (H - 4)).toFixed(1));
  }
  const fcx = (Math.log2(fc) - fmin) / (fmax - fmin) * W;
  const xOf = (f) => (Math.log2(f) - fmin) / (fmax - fmin) * W;
  const hzMarks = [100, 1000, 10000];
  return (
    <div className="filtviz">
      <svg className="viz filter" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
        {hzMarks.map((f, i) => <line key={i} x1={xOf(f).toFixed(1)} y1="0" x2={xOf(f).toFixed(1)} y2={H} className="hzgrid" />)}
        <polygon className="fillz" points={`0,${H} ${pts.join(" ")} ${W},${H}`} />
        <polyline points={pts.join(" ")} />
        <line x1={fcx.toFixed(1)} y1="0" x2={fcx.toFixed(1)} y2={H} className="mark" />
      </svg>
      <div className="hzscale">
        {hzMarks.map((f, i) => <span key={i} style={{ left: (xOf(f) / W * 100).toFixed(1) + "%" }}>{f >= 1000 ? (f / 1000) + "k" : f}</span>)}
        <span className="hzcur" style={{ left: Math.max(0, Math.min(100, fcx / W * 100)).toFixed(1) + "%" }}>{fc >= 1000 ? (fc / 1000).toFixed(1) + "k" : Math.round(fc)}</span>
      </div>
    </div>
  );
}

/* ---- FX hero visualizers (fill the FX panels, like a real effect view) ---- */
function jsSoftSat(x) { if (x < -3) return -1; if (x > 3) return 1; const x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }
function DriveCurve() {
  const drv = B.useSlider("fxDrive")[0];
  const W = 200, H = 100, N = 80, g = 1 + drv * 4, pts = [];
  for (let i = 0; i <= N; i++) { const x = -1 + 2 * i / N; const y = jsSoftSat(x * g); pts.push(((i / N) * W).toFixed(1) + "," + (H / 2 - y * (H / 2 - 4)).toFixed(1)); }
  return (
    <svg className="viz drivecv" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <line x1="0" y1={H / 2} x2={W} y2={H / 2} className="grid" />
      <line x1={W / 2} y1="0" x2={W / 2} y2={H} className="grid" />
      <polyline points={pts.join(" ")} />
    </svg>
  );
}
function DelayViz() {
  const mix = B.useSlider("fxDelay")[0], fb = B.useSlider("delayFeedback")[0];
  const W = 200, H = 100, taps = 9, bars = [];
  for (let n = 0; n < taps; n++) bars.push({ x: 6 + n * (W - 14) / taps, h: Math.max(0.02, Math.pow(fb, n) * Math.min(1, mix + 0.25)) });
  return (
    <svg className="viz delayv" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <line x1="0" y1={H - 2} x2={W} y2={H - 2} className="grid" />
      {bars.map((b, i) => <rect key={i} x={b.x.toFixed(1)} y={((H - 4) * (1 - b.h) + 2).toFixed(1)} width="5" height={((H - 4) * b.h).toFixed(1)} className="bar" />)}
    </svg>
  );
}
function ReverbViz() {
  const mix = B.useSlider("fxReverb")[0], size = B.useSlider("reverbSize")[0];
  const W = 200, H = 100, N = 72, decay = 1.5 + size * 7, pts = [];
  for (let i = 0; i <= N; i++) { const t = i / N; const y = Math.exp(-t * 6 / decay) * Math.min(1, mix + 0.25); pts.push((t * W).toFixed(1) + "," + ((H - 3) - (H - 6) * y).toFixed(1)); }
  return (
    <svg className="viz reverbv" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <polyline points={pts.join(" ")} />
      <polygon className="fillz" points={`0,${H} ${pts.join(" ")} ${W},${H}`} />
    </svg>
  );
}
function SweepViz({ rateId, depthId }) {
  const rate = B.useSlider(rateId)[0], depth = B.useSlider(depthId)[0];
  const W = 200, H = 100, N = 96, cy = H / 2, cyc = 1 + rate * 4, amp = (H / 2 - 5) * (0.2 + depth * 0.8), pts = [];
  for (let i = 0; i <= N; i++) { const t = i / N; pts.push((t * W).toFixed(1) + "," + (cy - Math.sin(2 * Math.PI * t * cyc) * amp).toFixed(1)); }
  return (
    <svg className="viz sweepv" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <line x1="0" y1={cy} x2={W} y2={cy} className="grid" />
      <polyline points={pts.join(" ")} />
    </svg>
  );
}

/* ============================ panels ============================ */
function OscPanel({ n }) {
  const p = "osc" + n;
  const [on] = B.useToggle(p + "On");
  const [type] = B.useChoice(p + "Type"); // 0 BLEP, 1 wavetable
  const wt = type === 1;
  return (
    <Panel title={"OSC " + n}>
      <div className="row between">
        <Switch id={p + "On"} label="ON" />
        <Seg id={p + "Type"} options={["BLEP", "WT"]} />
        <Seg id={p + "Foot"} options={FOOTAGE} />
      </div>
      {wt ? <WtTableSel id={p + "WtTable"} />
          : <WaveSelect morphId={p + "Morph"} pwId={p + "Pw"} />}
      <OscWave morphId={p + "Morph"} pwId={p + "Pw"} crushId={p + "Crush"} on={on}
               wt={wt} tableId={p + "WtTable"} wtMorphId={p + "WtMorph"} />
      <div className="knobs spread">
        {wt ? <Knob id={p + "WtMorph"} label="MORPH" small />
            : <Knob id={p + "Morph"} label="MORPH" small />}
        {wt ? <Knob id={p + "WtTable"} label="TABLE" small />
            : <Knob id={p + "Pw"} label="PW" small />}
        <Knob id={p + "Detune"} label="DETUNE" bipolar small />
        <Knob id={p + "Crush"} label="CRUSH" small />
        <Knob id={p + "Sr"} label="SR" small />
      </div>
    </Panel>
  );
}

function MixerPanel() {
  return (
    <Panel title="MIXER / SOURCES">
      <div className="cl">OSCILLATOR LEVELS</div>
      <div className="knobs spread">
        <Knob id="osc1Level" label="OSC 1" />
        <Knob id="osc2Level" label="OSC 2" />
        <Knob id="osc3Level" label="OSC 3" />
        <Knob id="osc4Level" label="OSC 4" />
        <Knob id="osc5Level" label="OSC 5" />
      </div>
      <div className="cl">SUB · NOISE · RING · FM · BALANCE</div>
      <div className="knobs spread">
        <Knob id="subLevel" label="SUB" />
        <Knob id="noiseLevel" label="NOISE" />
        <Knob id="ringMod" label="RING" />
        <Knob id="fm2to1" label="FM 2>1" />
        <Knob id="oscMix" label="1·2 BAL" bipolar />
      </div>
      <div className="row between">
        <Seg id="subOctave" options={["-1", "-2"]} label="SUB OCT" />
        <Seg id="sync2Mode" options={SYNC_MODES} label="O2 SYNC" />
        <Seg id="sync3Mode" options={SYNC_MODES} label="O3 SYNC" />
      </div>
    </Panel>
  );
}

function FilterPanel() {
  return (
    <Panel title="FILTER" accent="#5ad0e0">
      <div className="row between">
        <Seg id="filterMode" options={FILTER_MODES} label="MODE" />
        <Seg id="filterRouting" options={["SINGLE", "SER", "PAR"]} label="ROUTING" />
      </div>
      <FilterCurve />
      <div className="knobs spread">
        <Knob id="cutoff" label="CUTOFF" small />
        <Knob id="resonance" label="RESO" small />
        <Knob id="envFilterAmt" label="ENV>CUT" bipolar small />
        <Knob id="filterDrive" label="DRIVE" small />
        <Knob id="keyTrack" label="KEYTRK" small />
      </div>
      <FilterTwo />
    </Panel>
  );
}

function FilterTwo() {
  const [routing] = B.useChoice("filterRouting");
  if (routing === 0) return null; // single filter -> hide the 2nd
  return (
    <div className="flt2">
      <Seg id="filter2Mode" options={FILTER_MODES} label={"FILTER 2 · " + (routing === 1 ? "SERIES" : "PARALLEL")} />
      <div className="knobs spread">
        <Knob id="filter2Cutoff" label="CUTOFF" small />
        <Knob id="filter2Reso" label="RESO" small />
        <Knob id="filter2EnvAmt" label="ENV>CUT" bipolar small />
        <Knob id="filter2Drive" label="DRIVE" small />
      </div>
    </div>
  );
}

function EnvPanel({ n, name, accent }) {
  const p = "env" + n;
  return (
    <Panel title={name} accent={accent}>
      <EnvViz n={n} />
      <div className="knobs">
        <Knob id={p + "Attack"} label="A" />
        <Knob id={p + "Decay"} label="D" />
        <Knob id={p + "Sustain"} label="S" />
        <Knob id={p + "Release"} label="R" />
        <Knob id={p + "Vel"} label="VEL" />
      </div>
    </Panel>
  );
}

function LfoPanel({ n, accent }) {
  const p = "lfo" + n;
  const [sync] = B.useToggle(p + "Sync");
  const meters = B.useEvent("meters", { lfo: [0, 0, 0] });
  const lv = (meters.lfo && meters.lfo[n - 1]) || 0;
  return (
    <Panel title={"LFO " + n} accent={accent}>
      <LfoScope n={n} />
      <LfoWaveSelect id={p + "Wave"} />
      <div className="row between">
        <Switch id={p + "Sync"} label="SYNC" />
        <div className="lfo-tail">
          <Switch id={p + "Retrig"} label="RETRIG" />
          <Switch id={p + "PerVoice"} label="PER-V" />
          <div className="lfo-led" style={{ "--g": (lv * 0.5 + 0.5).toFixed(3) }} />
        </div>
      </div>
      <div className="knobs lfo3">
        {sync ? <Sel id={p + "Div"} options={DIVISIONS} label="DIV" />
              : <Knob id={p + "Rate"} label="RATE" small />}
        <Knob id={p + "Depth"} label="DEPTH" small />
        <Knob id={p + "Offset"} label="OFFSET" bipolar small />
        <Knob id={p + "Phase"} label="PHASE" small />
        <Knob id={p + "Fade"} label="FADE" small />
        <Knob id={p + "Delay"} label="DELAY" small />
      </div>
    </Panel>
  );
}

/* Abbreviated source names for the grid column headers (index-aligned to
   MOD_SRC; [0] is the unused OFF slot). */
const SRC_ABBR = ["", "L1", "L2", "L3", "AEG", "FEG", "XEG", "VEL", "KEY", "MW", "AT", "BND", "RND", "NOT", "MPR", "MTM", "MBN", "S1", "S2", "S3", "S4"];
const N_MOD_SLOTS = 32;

/* One grid cell = one (source, destination) intersection. Drag vertically to
   set a bipolar amount; the fill grows from the centre line and an active cell
   glows with the live source*amount contribution. */
function ModCell({ srcI, dstI, amt, live, onSet, def = 0 }) {
  const drag = useRef(null);
  const down = (e) => {
    e.preventDefault();
    const p = e.touches ? e.touches[0] : e;
    drag.current = { y: p.clientY, a: amt };
    const move = (ev) => {
      const q = ev.touches ? ev.touches[0] : ev;
      const fine = ev.shiftKey ? 0.25 : 1;
      onSet(srcI, dstI, drag.current.a + (drag.current.y - q.clientY) / 70 * fine);
    };
    const up = () => {
      window.removeEventListener("mousemove", move); window.removeEventListener("mouseup", up);
      window.removeEventListener("touchmove", move); window.removeEventListener("touchend", up);
    };
    window.addEventListener("mousemove", move); window.addEventListener("mouseup", up);
    window.addEventListener("touchmove", move, { passive: false }); window.addEventListener("touchend", up);
  };
  const mag = Math.min(1, Math.abs(amt) / 2);
  const factory = def !== 0 && Math.abs(amt) < 0.001; // pre-patched pin, no user override
  const cls = "mg-cell" + (amt > 0.001 ? " pos" : amt < -0.001 ? " neg" : "") + (factory ? " factory" : "");
  const lv = Math.min(1, Math.abs(live) / 1.2);
  return (
    <div className={cls} onMouseDown={down} onTouchStart={down}
         onDoubleClick={() => onSet(srcI, dstI, 0)}
         title={MOD_SRC[srcI] + " → " + MOD_DST[dstI] + (amt ? "   " + amt.toFixed(2) : (factory ? "   (factory " + def.toFixed(2) + ")" : ""))}
         style={{ "--m": mag.toFixed(3), "--lv": lv.toFixed(3) }}>
      <i />
    </div>
  );
}

function ModGrid() {
  const mc = React.useContext(ModContext);
  // read every sparse slot once; build a (src,dst) -> slot lookup
  const slots = [];
  for (let i = 1; i <= N_MOD_SLOTS; i++) {
    const [src, setSrc] = B.useChoice("mod" + i + "Src");
    const [dst, setDst] = B.useChoice("mod" + i + "Dst");
    const [amtN, setAmt] = B.useSlider("mod" + i + "Amt");
    slots.push({ src, dst, amtN, setSrc, setDst, setAmt });
  }
  const byPair = {};
  let used = 0;
  slots.forEach((s) => { if (s.src > 0 && s.dst > 0) { byPair[s.src + "_" + s.dst] = s; used++; } });

  const setCell = (srcI, dstI, amt) => {
    amt = Math.max(-2, Math.min(2, amt));
    const key = srcI + "_" + dstI;
    let s = byPair[key];
    if (!s) {
      if (Math.abs(amt) < 0.01) return;
      s = slots.find((x) => x.src <= 0); // allocate a free route
      if (!s) return;                    // matrix full (all 32 used)
      s.setSrc(srcI); s.setDst(dstI);
    }
    s.setAmt((amt + 2) / 4);
    if (Math.abs(amt) < 0.01) s.setSrc(0); // clearing frees the route
  };

  const srcCols = SRC_ABBR.slice(1);          // 13 sources
  const dstRows = MOD_DST.slice(1);           // 29 destinations
  return (
    <Panel title={"MOD MATRIX · " + used + "/" + N_MOD_SLOTS}>
      <div className="mgrid" style={{ "--cols": srcCols.length }}>
        <div className="mg-corner" />
        {srcCols.map((s, c) => <div key={c} className="mg-chead" title={MOD_SRC[c + 1]}>{s}</div>)}
        {dstRows.map((dname, r) => {
          const dstI = r + 1;
          return [
            <div key={"l" + r} className="mg-rlabel">{dname}</div>,
            ...srcCols.map((_, c) => {
              const srcI = c + 1;
              const slot = byPair[srcI + "_" + dstI];
              const amt = slot ? slot.amtN * 4 - 2 : 0;
              const live = slot ? ((mc.src && mc.src[srcI]) || 0) * amt : 0;
              return <ModCell key={r + "_" + c} srcI={srcI} dstI={dstI} amt={amt} live={live} onSet={setCell} />;
            }),
          ];
        })}
      </div>
    </Panel>
  );
}

/* EMS-style patchbay: a second routing bank over the full source x dest grid,
   with a pre-patched/manual toggle. Independent of the MOD matrix bank. */
/* modular audio patchbay: rows = audio source nodes, cols = destination buses,
   each cell a bipolar gain (ab{s}_{d}). Reroutes the actual signal topology when
   PATCHBAY ON; off = the classic osc->filter->VCA path. */
const ANODES = ["OSC1", "OSC2", "OSC3", "OSC4", "OSC5", "SUB", "NOISE", "RING", "SHAPER", "FLT1", "FLT2"];
const ADSTS  = ["FM1", "FM2", "FM3", "FM4", "FM5", "FLT1", "FLT2", "RING A", "RING B", "SHAPER", "OUT"];

function ABayCell({ s, d }) {
  const [v, set] = B.useSlider("ab" + s + "_" + d); // 0..1 of the -1..+1 range
  const amt = v * 2 - 1;
  const drag = useRef(null);
  const down = (e) => {
    e.preventDefault();
    const p = e.touches ? e.touches[0] : e;
    drag.current = { y: p.clientY, a: amt };
    const move = (ev) => {
      const q = ev.touches ? ev.touches[0] : ev;
      const fine = ev.shiftKey ? 0.25 : 1;
      const na = Math.max(-1, Math.min(1, drag.current.a + (drag.current.y - q.clientY) / 120 * fine));
      set((na + 1) / 2);
    };
    const up = () => {
      window.removeEventListener("mousemove", move); window.removeEventListener("mouseup", up);
      window.removeEventListener("touchmove", move); window.removeEventListener("touchend", up);
    };
    window.addEventListener("mousemove", move); window.addEventListener("mouseup", up);
    window.addEventListener("touchmove", move, { passive: false }); window.addEventListener("touchend", up);
  };
  const cls = "mg-cell" + (amt > 0.004 ? " pos" : amt < -0.004 ? " neg" : "");
  return (
    <div className={cls} onMouseDown={down} onTouchStart={down} onDoubleClick={() => set(0.5)}
         title={ANODES[s] + " → " + ADSTS[d] + (Math.abs(amt) > 0.004 ? "   " + amt.toFixed(2) : "")}
         style={{ "--m": Math.min(1, Math.abs(amt)).toFixed(3) }}><i /></div>
  );
}

function Patchbay() {
  const [on] = B.useToggle("patchbayOn");
  return (
    <Panel title="PATCHBAY · AUDIO TOPOLOGY">
      <div className="row between pbhead">
        <Switch id="patchbayOn" label="PATCHBAY ON" />
        <span className="pb-hint">{on ? "modular routing active — the fixed path is bypassed" : "off — classic osc → filter → VCA path"}</span>
      </div>
      <div className={"abgrid" + (on ? "" : " dim")} style={{ "--cols": ADSTS.length }}>
        <div className="mg-corner" />
        {ADSTS.map((d, c) => <div key={c} className="mg-chead">{d}</div>)}
        {ANODES.map((sname, s) => ([
          <div key={"l" + s} className="mg-rlabel">{sname}</div>,
          ...ADSTS.map((_, d) => <ABayCell key={s + "_" + d} s={s} d={d} />),
        ]))}
      </div>
    </Panel>
  );
}

function ArpPanel() {
  const [on] = B.useToggle("arpOn");
  return (
    <Panel title="ARP" accent="#e07a9a">
      <div className="row">
        <Switch id="arpOn" label="ON" />
        <Switch id="arpLatch" label="LATCH" />
        <Sel id="arpMode" options={ARP_MODES} label="MODE" />
      </div>
      <div className={"row" + (on ? "" : " dim")}>
        <Sel id="arpSyncDiv" options={DIVISIONS} label="DIV" />
        <Knob id="arpGate" label="GATE" />
        <Knob id="arpSwing" label="SWING" />
      </div>
      <div className={"row" + (on ? "" : " dim")}>
        <IntSeg id="arpOctaves" label="OCT" min={1} max={4} />
        <IntSeg id="arpRatchet" label="RATCH" min={1} max={4} />
      </div>
    </Panel>
  );
}

function DrivePanel() {
  return (
    <Panel title="DRIVE · MULTIBAND SAT">
      <DriveCurve />
      <div className="knobs spread">
        <Knob id="fxDrive" label="DRIVE" small />
        <Knob id="driveTone" label="TONE" bipolar small />
        <Knob id="mbLow" label="MB LOW" small />
        <Knob id="mbMid" label="MB MID" small />
        <Knob id="mbHigh" label="MB HIGH" small />
      </div>
      <Seg id="driveMode" options={["SOFT", "TUBE", "DIODE", "FOLD", "FUZZ"]} label="MODEL" />
    </Panel>
  );
}

function ChorusPanel() {
  return (
    <Panel title="CHORUS">
      <SweepViz rateId="chorusRate" depthId="chorusDepth" />
      <div className="knobs spread">
        <Knob id="fxChorus" label="MIX" />
        <Knob id="chorusRate" label="RATE" />
        <Knob id="chorusDepth" label="DEPTH" />
      </div>
      <Seg id="chorusMode" options={["I", "II", "ENS", "I+II"]} label="MODE" />
    </Panel>
  );
}

function PhaserPanel() {
  return (
    <Panel title="PHASER">
      <SweepViz rateId="phaserRate" depthId="phaserDepth" />
      <div className="knobs spread">
        <Knob id="fxPhaser" label="MIX" />
        <Knob id="phaserRate" label="RATE" />
        <Knob id="phaserDepth" label="DEPTH" />
        <Knob id="phaserFeedback" label="FBK" />
      </div>
    </Panel>
  );
}

function DelayPanel() {
  const [sync] = B.useToggle("delaySync");
  return (
    <Panel title="DELAY" accent="#6ac0d0">
      <DelayViz />
      <div className="knobs spread">
        <Knob id="fxDelay" label="MIX" small />
        {sync ? <Sel id="delayDiv" options={DIVISIONS} label="TIME" />
              : <Knob id="delayTimeMs" label="TIME" small />}
        <Knob id="delayFeedback" label="FBK" small />
        <Knob id="delayTone" label="TONE" small />
        <Knob id="delayFltFreq" label="FLT FREQ" small />
        <Knob id="delayFltQ" label="FLT Q" small />
      </div>
      <div className="row between">
        <Seg id="delayMode" options={["DIGI", "ANLG", "TAPE"]} label="MODE" />
        <Seg id="delayFltType" options={["LP", "HP", "BP"]} label="FILTER" />
        <Switch id="delaySync" label="SYNC" />
        <Switch id="delayPing" label="PING" />
      </div>
    </Panel>
  );
}

function ReverbPanel() {
  return (
    <Panel title="REVERB" accent="#7a9ad0">
      <ReverbViz />
      <div className="knobs spread">
        <Knob id="fxReverb" label="MIX" />
        <Knob id="reverbSize" label="SIZE" />
        <Knob id="reverbTone" label="TONE" />
      </div>
    </Panel>
  );
}

function VoicingPanel() {
  const [mode] = B.useChoice("voiceMode");
  const uni = mode === 2;
  return (
    <Panel title="VOICING">
      <div className="row between">
        <Seg id="voiceMode" options={VOICE_MODES} label="MODE" />
        <IntPick id="maxVoices" label="MAX POLY" min={1} max={16} />
      </div>
      <div className="knobs spread">
        <Knob id="glideTime" label="GLIDE" />
        <Knob id="drift" label="DRIFT" />
        <Knob id="bendRange" label="BEND" />
      </div>
      <div className={"knobs spread uni" + (uni ? "" : " dim")}>
        <Knob id="unisonDetune" label="UNI DET" />
        <Knob id="unisonSpread" label="UNI SPR" />
        <IntSeg id="unisonCount" label="VOICES" min={1} max={7} />
      </div>
      <div className="row between">
        <Seg id="glideMode" options={GLIDE_MODES} label="GLIDE MODE" />
        <Switch id="mpeOn" label="MPE" />
        <IntPick id="mpeBendRange" label="MPE BEND" min={1} max={96} />
      </div>
    </Panel>
  );
}

function MasterPanel() {
  const meters = B.useEvent("meters", { outL: 0, outR: 0 });
  const bar = (x) => Math.max(0, Math.min(1, Math.sqrt(x))) * 100; // perceptual
  return (
    <Panel title="MASTER">
      <div className="knobs spread">
        <Knob id="ampGain" label="LEVEL" big />
        <Knob id="width" label="WIDTH" />
        <Knob id="pan" label="PAN" bipolar />
        <Knob id="masterTune" label="TUNE" bipolar />
      </div>
      <div className="vu">
        <div className="vu-row"><span className="vu-l">L</span>
          <div className="vu-bar"><i style={{ width: bar(meters.outL) + "%" }} /></div></div>
        <div className="vu-row"><span className="vu-l">R</span>
          <div className="vu-bar"><i style={{ width: bar(meters.outR) + "%" }} /></div></div>
      </div>
    </Panel>
  );
}

/* ============================ top bar ============================ */
function VoiceLeds() {
  const meters = B.useEvent("meters", { voiceLevels: [] });
  const [mv] = B.useSlider("maxVoices");
  const n = Math.round(1 + mv * 15); // maxVoices 1..16
  const lv = meters.voiceLevels || [];
  const leds = [];
  for (let i = 0; i < n; i++) {
    const g = Math.max(0, Math.min(1, lv[i] || 0));
    leds.push(<span key={i} className={"vled" + (g > 0.02 ? " on" : "")} style={{ "--g": g.toFixed(3) }} />);
  }
  return (
    <div className="tb-voices" title={n + " voices"}>
      <div className="vleds">{leds}</div>
      <span className="vl">VOICES</span>
    </div>
  );
}

function TopBar() {
  const preset = B.useEvent("preset", { name: "INIT", category: 1, index: -1 });
  const meters = B.useEvent("meters", { voices: 0, note: -1 });
  const [browse, setBrowse] = useState(false);
  const [saving, setSaving] = useState(false);
  const prev = B.nativeFn("presetPrev"), next = B.nativeFn("presetNext"), init = B.nativeFn("initPatch");
  return (
    <header className="topbar">
      <div className="brand"><span className="logo">JOVE</span><span className="sub">POLYSYNTH</span></div>
      <div className="preset">
        <button className="nav" onClick={() => prev()}>{"‹"}</button>
        <button className="pname" onClick={() => setBrowse(true)}>
          <span className="cat">{CATEGORIES[preset.category] || ""}</span>
          <span className="nm">{preset.name}</span>
        </button>
        <button className="nav" onClick={() => next()}>{"›"}</button>
      </div>
      <div className="tb-actions">
        <button className="tbtn" onClick={() => init()}>INIT</button>
        <button className="tbtn" onClick={() => setSaving(true)}>SAVE</button>
        <button className="tbtn" onClick={() => setBrowse(true)}>BROWSE</button>
      </div>
      <div className="tb-fx">
        <span className="cl">FX</span>
        <div className="tb-fxrow">
          <FxBtn id="fxDriveOn" label="DRV" />
          <FxBtn id="fxChorusOn" label="CHO" />
          <FxBtn id="fxPhaserOn" label="PHA" />
          <FxBtn id="fxDelayOn" label="DLY" />
          <FxBtn id="fxReverbOn" label="REV" />
        </div>
      </div>
      <div className="tb-global">
        <Seg id="quality" options={QUALITY} label="QUALITY" />
      </div>
      <VoiceLeds />
      {browse && <Browser onClose={() => setBrowse(false)} current={preset.index} />}
      {saving && <SaveDialog onClose={() => setSaving(false)} cat={preset.category} />}
    </header>
  );
}

function Browser({ onClose, current }) {
  const [list, setList] = useState([]);
  const [cat, setCat] = useState(-1);
  const [q, setQ] = useState("");
  useEffect(() => { B.nativeFn("listPresets")().then((r) => setList(r || [])); }, []);
  const load = B.nativeFn("loadPreset");
  const ql = q.trim().toLowerCase();
  const inCat = (e) => cat === -2 ? !e.factory : (cat < 0 || e.category === cat);
  const shown = list.filter((e) => inCat(e) && (!ql || e.name.toLowerCase().indexOf(ql) >= 0));
  const userCount = list.filter((e) => !e.factory).length;
  // factory index == MIDI program change (bank 0); shown 0-based to match the PC value
  const pc = (e) => e.factory ? String(e.index).padStart(3, "0") : "U";
  return (
    <div className="modal" onClick={onClose}>
      <div className="sheet" onClick={(e) => e.stopPropagation()}>
        <div className="sheet-h">
          <b>PRESETS</b>
          <input className="ti search" autoFocus placeholder="search…" value={q}
                 onChange={(e) => setQ(e.target.value)} />
          <button onClick={onClose}>{"✕"}</button>
        </div>
        <div className="cats">
          <button className={cat < 0 ? "on" : ""} onClick={() => setCat(-1)}>ALL</button>
          {CATEGORIES.map((c, i) =>
            <button key={i} className={cat === i ? "on" : ""} onClick={() => setCat(i)}>{c}</button>)}
          {userCount > 0 &&
            <button className={"usercat" + (cat === -2 ? " on" : "")} onClick={() => setCat(-2)}>USER ({userCount})</button>}
        </div>
        <div className="plist">
          {shown.map((e) =>
            <button key={e.index} className={"pitem" + (e.index === current ? " sel" : "") + (e.factory ? "" : " user")}
                    onClick={() => { load(e.index); }}>
              <span className="pi-num">{pc(e)}</span>
              <span className="pi-cat">{CATEGORIES[e.category]}</span>
              <span className="pi-nm">{e.name}</span>
            </button>)}
        </div>
        <div className="sheet-f">{shown.length} presets · # = MIDI program change (bank 0)</div>
      </div>
    </div>
  );
}

function SaveDialog({ onClose, cat }) {
  const [name, setName] = useState("");
  const [c, setC] = useState(cat || 0);
  const save = B.nativeFn("savePreset");
  return (
    <div className="modal" onClick={onClose}>
      <div className="sheet small" onClick={(e) => e.stopPropagation()}>
        <div className="sheet-h"><b>SAVE PRESET</b><button onClick={onClose}>{"✕"}</button></div>
        <input className="ti" autoFocus placeholder="patch name" value={name}
               onChange={(e) => setName(e.target.value)} />
        <div className="sel-wrap" style={{ margin: "0 18px" }}>
          <Dropdown idx={c} options={CATEGORIES} onPick={setC} />
        </div>
        <button className="tbtn save" disabled={!name.trim()}
                onClick={() => { save(name.trim(), c); onClose(); }}>SAVE</button>
      </div>
    </div>
  );
}

/* ============================ app ============================ */
/* A sequencer's step lane is a paint surface: click + drag horizontally to draw
   values across steps (vertical position = value); drag on one step to set just
   it; double-click clears a step to centre. */
function SeqPanel({ n }) {
  const p = "seq" + n;
  const [sync] = B.useToggle(p + "Sync");
  const [mode] = B.useChoice(p + "Mode");
  const melodic = mode === 1;
  const lenV = B.useSlider(p + "Len");
  const L = Math.max(1, Math.round(lenV[2] ? lenV[2]() : 16));
  const meters = B.useEvent("meters", { seqStep: [0, 0, 0, 0] });
  const play = (meters.seqStep && meters.seqStep[n - 1]) || 0;
  const steps = [];
  for (let s = 0; s < 16; s++) steps.push(B.useSlider(p + "step" + (s + 1)));
  const laneRef = useRef(null);
  const paint = (clientX, clientY, snap) => {
    const el = laneRef.current; if (!el) return;
    const r = el.getBoundingClientRect();
    const col = Math.floor(((clientX - r.left) / r.width) * 16);
    if (col < 0 || col > 15) return;
    let v = 1 - (clientY - r.top) / r.height; // top = +1, bottom = -1 (normalized 0..1)
    v = Math.max(0, Math.min(1, v));
    if (snap) v = Math.round((v * 2 - 1) * 12) / 12 / 2 + 0.5; // melodic: snap to semitone
    steps[col][1](v);
  };
  const down = (e) => {
    e.preventDefault();
    const pt = e.touches ? e.touches[0] : e;
    paint(pt.clientX, pt.clientY, melodic);
    const move = (ev) => { const q = ev.touches ? ev.touches[0] : ev; paint(q.clientX, q.clientY, melodic); };
    const up = () => {
      window.removeEventListener('mousemove', move); window.removeEventListener('mouseup', up);
      window.removeEventListener('touchmove', move); window.removeEventListener('touchend', up);
    };
    window.addEventListener('mousemove', move); window.addEventListener('mouseup', up);
    window.addEventListener('touchmove', move, { passive: false }); window.addEventListener('touchend', up);
  };
  const dbl = (e) => {
    const el = laneRef.current; if (!el) return;
    const r = el.getBoundingClientRect();
    const col = Math.floor(((e.clientX - r.left) / r.width) * 16);
    if (col >= 0 && col <= 15) steps[col][1](0.5);
  };
  return (
    <Panel title={"SEQ " + n}>
      <div className="seqgrid" ref={laneRef} onMouseDown={down} onTouchStart={down} onDoubleClick={dbl}>
        {steps.map((st, s) => {
          const amt = st[0] * 2 - 1;
          const cls = "sq-cell" + (amt > 0.004 ? " pos" : amt < -0.004 ? " neg" : "") + (s === play && s < L ? " play" : "") + (s >= L ? " dimstep" : "");
          return <div key={s} className={cls} style={{ "--m": Math.abs(amt).toFixed(3) }}><i /></div>;
        })}
      </div>
      <div className="row seqctl">
        <Switch id={p + "Sync"} label="SYNC" />
        {sync ? <Sel id={p + "Div"} options={DIVISIONS} label="DIV" />
              : <Knob id={p + "Rate"} label="RATE" small />}
        <IntPick id={p + "Len"} label="LEN" min={1} max={16} />
        <Seg id={p + "Dir"} options={SEQ_DIRS} label="DIR" />
        <Seg id={p + "Mode"} options={SEQ_MODES} label="MODE" />
        <Seg id={p + "Curve"} options={SEQ_CURVES} label="CURVE" />
        <Knob id={p + "Depth"} label="DEPTH" small />
        <Knob id={p + "Swing"} label="SWING" small />
        <Switch id={p + "Retrig"} label="RTRG" />
      </div>
    </Panel>
  );
}

const TABS = [["voice", "VOICE"], ["mod", "MOD"], ["seq", "SEQ"], ["patch", "PATCH"], ["fx", "FX · ARP"]];

function App() {
  const modMap = useModMap();
  const meters = B.useEvent("meters", { lfo: [0, 0, 0], modSrc: [] });
  const modCtx = useMemo(() => ({ map: modMap, lfo: meters.lfo || [0, 0, 0], src: meters.modSrc || [] }),
                         [modMap, meters.lfo, meters.modSrc]);
  const [tab, setTab] = useState("voice");
  const cols = (t) => "cols" + (tab === t ? "" : " hide");
  return (
    <ModContext.Provider value={modCtx}>
    <div id="app">
      <TopBar />
      <nav className="tabbar">
        {TABS.map(([id, label]) =>
          <button key={id} className={"tab" + (tab === id ? " on" : "")} onClick={() => setTab(id)}>{label}</button>)}
        <span className="tab-spacer" />
        <span className="brandmark">{window.JOVE_VERSION_STR || ""}</span>
      </nav>

      {/* all tabs stay mounted (relays stay bound); inactive ones are hidden */}
      <div className={"voicetab" + (tab === "voice" ? "" : " hide")}>
        <section className="band oscband">
          <header className="band-h">OSCILLATORS</header>
          <div className="oscrow">
            <OscPanel n={1} />
            <OscPanel n={2} />
            <OscPanel n={3} />
            <OscPanel n={4} />
            <OscPanel n={5} />
          </div>
        </section>
        <div className="band botband">
          <MixerPanel />
          <FilterPanel />
          <VoicingPanel />
          <div className="rightstack">
            <EnvPanel n={1} name="AMP ENV" accent="#e0a85a" />
            <EnvPanel n={2} name="FILTER ENV" accent="#5ad0e0" />
            <MasterPanel />
          </div>
        </div>
      </div>

      <div className={"modtab" + (tab === "mod" ? "" : " hide")}>
        {/* envelopes + LFOs on a 2x3 grid so each row's pair aligns exactly */}
        <div className="modgrid2">
          <EnvPanel n={1} name="AMP ENV" />
          <LfoPanel n={1} />
          <EnvPanel n={2} name="FILTER ENV" />
          <LfoPanel n={2} />
          <EnvPanel n={3} name="AUX ENV" />
          <LfoPanel n={3} />
        </div>
        <div className="col modcol">
          <ModGrid />
        </div>
      </div>

      <div className={"seqtab" + (tab === "seq" ? "" : " hide")}>
        <div className="seqgrid2">
          <SeqPanel n={1} /><SeqPanel n={2} /><SeqPanel n={3} /><SeqPanel n={4} />
        </div>
      </div>

      <div className={"patchtab" + (tab === "patch" ? "" : " hide")}>
        <Patchbay />
      </div>

      <div className={"fxtab" + (tab === "fx" ? "" : " hide")}>
        <div className="fxgrid">
          <DrivePanel />
          <ChorusPanel />
          <PhaserPanel />
          <DelayPanel />
          <ReverbPanel />
          <ArpPanel />
        </div>
      </div>

      <footer className="foot">
        <span>{window.JOVE_VERSION_STR || ""}</span>
        <span>DatanoiseTV</span>
      </footer>
    </div>
    </ModContext.Provider>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
