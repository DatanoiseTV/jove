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
const FILTER_MODES= ["MOOG", "LP", "HP", "BP", "NOTCH"];
const ARP_MODES   =["UP","DOWN","UP-DN","UP-DN+","DN-UP","PINGPONG","CONV","DIV","CON-DIV","ASPLAYED","RANDOM","CHORD"];
const DIVISIONS   = ["1/64","1/32T","1/32","1/16T","1/16","1/8T","1/16.","1/8","1/4T","1/8.","1/4","1/4.","1/2"];
const MOD_SRC     = ["OFF","LFO1","LFO2","LFO3","AMP EG","FLT EG","AUX EG","VEL","KEY","MWHEEL","ATOUCH","BEND","RAND","NOTE"];
const MOD_DST     = ["OFF","PITCH","O2 PIT","O3 PIT","MORPH1","MORPH2","MORPH3","PW1","PW2","PW3","OSCMIX","SUB","NOISE","CUTOFF","RESO","FDRIVE","AMP","PAN","L1 RATE","L2 RATE","L3 RATE","L1 DPT","L2 DPT","L3 DPT","FXSEND","FXPARM","FM","RING","ENVFLT","DETUNE"];
const CHORUS_MODES= ["Chorus I", "Chorus II", "Ensemble"];
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

/* Read all 10 mod slots and build paramId -> [{src, amt}] for the indicators. */
function useModMap() {
  const slots = [];
  for (let i = 1; i <= 10; i++) {
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

function Knob({ id, label, bipolar = false, big = false }) {
  const [v, set, scaled] = B.useSlider(id);
  const ref = useRef(null);
  const drag = useRef(null);
  const A0 = -135, SWEEP = 270;
  const D = big ? 58 : 48, c = D / 2, sw = big ? 4 : 3.2, R = c - sw - 1;
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
function OscWave({ morphId, pwId, on }) {
  const m = B.useSlider(morphId)[0];
  const pw = B.useSlider(pwId)[0];
  const W = 160, H = 40, cy = H / 2, amp = H / 2 - 3, N = 96, cyc = 2;
  const pts = [];
  for (let i = 0; i <= N; i++) { const t = (i / N) * cyc; pts.push(((i / N) * W).toFixed(1) + "," + (cy - oscShape(m, pw, t) * amp).toFixed(1)); }
  return (
    <svg className={"viz oscwave" + (on ? "" : " off")} viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
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
  return (
    <svg className="viz env" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <polyline points={pts.map((p) => p[0].toFixed(1) + "," + p[1].toFixed(1)).join(" ")} />
      <polygon className="fillz" points={`${pad},${H - pad} ${pts.map((p) => p[0].toFixed(1) + "," + p[1].toFixed(1)).join(" ")} ${(x).toFixed(1)},${H - pad}`} />
    </svg>
  );
}

// Draw the LFO's actual waveform shape (two cycles) from its wave type — exact at
// any rate, unlike a 30 Hz rolling scope which aliases. A live playhead dot rides
// the curve at the engine-reported phase so motion is still visible and correct.
function LfoScope({ n }) {
  const [wave] = B.useChoice("lfo" + n + "Wave");
  const meters = B.useEvent("meters", { lfoPhase: [0, 0, 0] });
  const ph = (meters.lfoPhase && meters.lfoPhase[n - 1]) || 0;
  const W = 140, H = 26, cy = H / 2, amp = H / 2 - 2.5, cycles = 2;
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
    const steps = 4 * cycles;
    pointStr = stairPoints(steps, W, cy, amp);
    const si = Math.min(steps - 1, Math.floor(phPos * steps));
    phy = cy - SNH_LEVELS[si % SNH_LEVELS.length] * amp;
  } else {
    const N = 96, pts = [];
    for (let i = 0; i <= N; i++) {
      const t = (i / N) * cycles;
      pts.push(((i / N) * W).toFixed(1) + "," + (cy - shape(t) * amp).toFixed(1));
    }
    pointStr = pts.join(" ");
    phy = cy - shape(ph * cycles) * amp;
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
  const [mode] = B.useChoice("filterMode");
  const W = 180, H = 38, N = 72;
  const fmin = Math.log2(20), fmax = Math.log2(20000);
  const fc = 20 * Math.pow(2, cut * 9.6);          // engine cutoff mapping (Hz)
  const Q = 0.5 + res * 11;                          // resonance -> Q
  const ladder = mode === 0;
  const mag = (f) => {
    const w = f / fc, w2 = w * w;
    const denom = Math.sqrt((1 - w2) * (1 - w2) + (w / Q) * (w / Q)) || 1e-9;
    let h;
    if (mode === 0 || mode === 1) h = 1 / denom;        // LP (moog/svf)
    else if (mode === 2) h = w2 / denom;                 // HP
    else if (mode === 3) h = (w / Q) / denom;            // BP
    else h = Math.abs(1 - w2) / denom;                   // notch
    if (ladder) h *= h;                                  // ~4-pole steepness
    return h;
  };
  const pts = [];
  for (let i = 0; i <= N; i++) {
    const f = Math.pow(2, fmin + (i / N) * (fmax - fmin));
    const db = 20 * Math.log10(Math.max(1e-3, mag(f)));  // ~-60..+20 dB
    const y = Math.max(0, Math.min(1, (db + 42) / 54));   // -42dB->0, +12dB->1
    pts.push(((i / N) * W).toFixed(1) + "," + (H - 2 - y * (H - 4)).toFixed(1));
  }
  const fcx = (Math.log2(fc) - fmin) / (fmax - fmin) * W;
  return (
    <svg className="viz filter" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <polyline points={pts.join(" ")} />
      <line x1={fcx.toFixed(1)} y1="0" x2={fcx.toFixed(1)} y2={H} className="mark" />
    </svg>
  );
}

/* ============================ panels ============================ */
function OscPanel({ n }) {
  const p = "osc" + n;
  const [on] = B.useToggle(p + "On");
  return (
    <Panel title={"OSC " + n}>
      <div className="row between">
        <Switch id={p + "On"} label="ON" />
        <Seg id={p + "Foot"} options={FOOTAGE} />
      </div>
      <WaveSelect morphId={p + "Morph"} pwId={p + "Pw"} />
      <OscWave morphId={p + "Morph"} pwId={p + "Pw"} on={on} />
      <div className="knobs spread">
        <Knob id={p + "Morph"} label="MORPH" />
        <Knob id={p + "Pw"} label="PW" />
        <Knob id={p + "Detune"} label="DETUNE" bipolar />
        <Knob id={p + "Level"} label="LEVEL" />
      </div>
    </Panel>
  );
}

function MixerPanel() {
  return (
    <Panel title="MIXER / SOURCES" accent="#e0a05a">
      <div className="knobs">
        <Knob id="oscMix" label="OSC1-2" bipolar />
        <Knob id="subLevel" label="SUB" />
        <Knob id="noiseLevel" label="NOISE" />
        <Knob id="ringMod" label="RING" />
        <Knob id="fm2to1" label="FM 2>1" />
      </div>
      <div className="row">
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
      <div className="row">
        <Seg id="filterMode" options={FILTER_MODES} label="MODE" />
        <FilterCurve />
      </div>
      <div className="knobs">
        <Knob id="cutoff" label="CUTOFF" big />
        <Knob id="resonance" label="RESO" big />
        <Knob id="envFilterAmt" label="ENV>CUT" />
        <Knob id="filterDrive" label="DRIVE" />
        <Knob id="keyTrack" label="KEYTRK" />
      </div>
    </Panel>
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
      <div className="row">
        <Switch id={p + "Sync"} label="SYNC" />
        <div className="lfo-led" style={{ "--g": (lv * 0.5 + 0.5).toFixed(3) }} />
      </div>
      <div className="knobs">
        {sync ? <Sel id={p + "Div"} options={DIVISIONS} label="DIV" />
              : <Knob id={p + "Rate"} label="RATE" />}
        <Knob id={p + "Depth"} label="DEPTH" />
        <Knob id={p + "Offset"} label="OFFSET" bipolar />
        <Knob id={p + "Fade"} label="FADE" />
        <Knob id={p + "Delay"} label="DELAY" />
      </div>
      <div className="row">
        <Switch id={p + "Retrig"} label="RETRIG" />
        <Switch id={p + "PerVoice"} label="PER-V" />
      </div>
    </Panel>
  );
}

function ModRow({ n }) {
  const p = "mod" + n;
  const [src] = B.useChoice(p + "Src");
  const mc = React.useContext(ModContext);
  const v = (src > 0 && mc.src) ? (mc.src[src] || 0) : 0;
  const mag = Math.min(1, Math.abs(v)) * 50;
  const fill = v >= 0 ? { bottom: "50%", height: mag + "%" } : { top: "50%", height: mag + "%" };
  return (
    <div className={"modrow" + (src > 0 ? " active" : "")}>
      <span className="mn">{n}</span>
      <div className="smeter"><i style={fill} /></div>
      <Sel id={p + "Src"} options={MOD_SRC} />
      <span className="arr">{"→"}</span>
      <Sel id={p + "Dst"} options={MOD_DST} />
      <Knob id={p + "Amt"} label="" bipolar />
    </div>
  );
}

function ModMatrix() {
  return (
    <Panel title="MOD MATRIX" accent="#c08ae0">
      <div className="modgrid">
        {[1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map((n) => <ModRow key={n} n={n} />)}
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

function FxPanel() {
  return (
    <Panel title="DRIVE / CHORUS / PHASER" accent="#8ae0a8">
      <div className="knobs">
        <Knob id="fxDrive" label="DRIVE" />
        <Knob id="fxChorus" label="CHORUS" />
        <Knob id="fxPhaser" label="PHASER" />
      </div>
      <Seg id="chorusMode" options={CHORUS_MODES} label="CHORUS MODE" />
    </Panel>
  );
}

function DelayPanel() {
  const [sync] = B.useToggle("delaySync");
  return (
    <Panel title="DELAY" accent="#6ac0d0">
      <div className="knobs">
        <Knob id="fxDelay" label="MIX" />
        {sync ? <Sel id="delayDiv" options={DIVISIONS} label="TIME" />
              : <Knob id="delayTimeMs" label="TIME" />}
        <Knob id="delayFeedback" label="FBK" />
        <Knob id="delayTone" label="TONE" />
      </div>
      <div className="row">
        <Switch id="delaySync" label="SYNC" />
        <Switch id="delayPing" label="PING-PONG" />
      </div>
    </Panel>
  );
}

function ReverbPanel() {
  return (
    <Panel title="REVERB" accent="#7a9ad0">
      <div className="knobs">
        <Knob id="fxReverb" label="MIX" />
        <Knob id="reverbSize" label="SIZE" />
        <Knob id="reverbTone" label="TONE" />
      </div>
    </Panel>
  );
}

function VoicingPanel() {
  const [mode] = B.useChoice("voiceMode");
  return (
    <Panel title="VOICING" accent="#9aa6e0">
      <div className="row">
        <Seg id="voiceMode" options={VOICE_MODES} label="MODE" />
        <IntPick id="maxVoices" label="MAX POLY" min={1} max={16} />
      </div>
      <div className="knobs">
        <Knob id="glideTime" label="GLIDE" />
        <Knob id="drift" label="DRIFT" />
        {mode === 2 && <Knob id="unisonDetune" label="UNI DET" />}
        {mode === 2 && <Knob id="unisonSpread" label="UNI SPR" />}
      </div>
      <div className="row">
        <Seg id="glideMode" options={GLIDE_MODES} label="GLIDE" />
        {mode === 2 && <IntSeg id="unisonCount" label="UNISON" min={1} max={7} />}
      </div>
    </Panel>
  );
}

function MasterPanel() {
  const meters = B.useEvent("meters", { outL: 0, outR: 0 });
  const bar = (x) => Math.max(0, Math.min(1, x)) * 100;
  return (
    <Panel title="MASTER" accent="#d8dee9">
      <div className="knobs">
        <Knob id="ampGain" label="LEVEL" big />
        <Knob id="width" label="WIDTH" />
        <Knob id="pan" label="PAN" bipolar />
        <Knob id="bendRange" label="BEND" />
      </div>
      <div className="vu">
        <div className="vu-bar"><i style={{ width: bar(meters.outL) + "%" }} /></div>
        <div className="vu-bar"><i style={{ width: bar(meters.outR) + "%" }} /></div>
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
const TABS = [["voice", "VOICE"], ["modfx", "MOD · FX · ARP"]];

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
          <MasterPanel />
        </div>
      </div>

      <div className={"modtab" + (tab === "modfx" ? "" : " hide")}>
        {/* envelopes + LFOs on a 2x3 grid so each row's pair aligns exactly */}
        <div className="modgrid2">
          <EnvPanel n={1} name="AMP ENV" />
          <LfoPanel n={1} />
          <EnvPanel n={2} name="FILTER ENV" />
          <LfoPanel n={2} />
          <EnvPanel n={3} name="AUX ENV" />
          <LfoPanel n={3} />
        </div>
        <div className="col fxcol">
          <FxPanel />
          <DelayPanel />
          <ReverbPanel />
          <ArpPanel />
        </div>
        <div className="col modcol">
          <ModMatrix />
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
