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
const LFO_WAVES   = ["SINE", "TRI", "SAW+", "SAW-", "SQR", "S&H"];
const ARP_MODES   = ["UP","DOWN","UP-DN","UP-DN+","DN-UP","PINGPONG","CONV","DIV","CON-DIV","ASPLAYED","RANDOM","CHORD"];
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
  const D = big ? 52 : 42, c = D / 2, sw = big ? 3.6 : 3, R = c - sw - 1;
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

function Sel({ id, options, label }) {
  const [idx, set] = B.useChoice(id);
  return (
    <div className="sel-wrap">
      {label && <div className="cl">{label}</div>}
      <select className="sel" value={idx} onChange={(e) => set(+e.target.value)}>
        {options.map((o, i) => <option key={i} value={i}>{o}</option>)}
      </select>
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
  const opts = []; for (let i = min; i <= max; i++) opts.push(i);
  return (
    <div className="sel-wrap">
      {label && <div className="cl">{label}</div>}
      <select className="sel" value={cur} onChange={(e) => set((+e.target.value - min) / (max - min))}>
        {opts.map((i) => <option key={i} value={i}>{i}</option>)}
      </select>
    </div>
  );
}

function Panel({ title, accent, children, wide, tall }) {
  return (
    <section className={"panel" + (wide ? " wide" : "") + (tall ? " tall" : "")}
             style={accent ? { "--accent": accent } : null}>
      <header className="ph"><span className="dot" />{title}</header>
      <div className="pbody">{children}</div>
    </section>
  );
}

/* ============================ visualizers ============================ */
function EnvViz({ n }) {
  const [a] = B.useSlider("env" + n + "Attack");
  const [d] = B.useSlider("env" + n + "Decay");
  const [s] = B.useSlider("env" + n + "Sustain");
  const [r] = B.useSlider("env" + n + "Release");
  const W = 140, H = 40, pad = 3;
  const aw = 0.05 + a, dw = 0.05 + d, sw = 0.34, rw = 0.05 + r;
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
  const W = 140, H = 26, cy = H / 2, amp = H / 2 - 2.5;
  const shape = (t) => {
    t = t - Math.floor(t);
    switch (wave) {
      case 0: return Math.sin(2 * Math.PI * t);
      case 1: return t < 0.5 ? 4 * t - 1 : 3 - 4 * t;
      case 2: return 2 * t - 1;
      case 3: return 1 - 2 * t;
      case 4: return t < 0.5 ? 1 : -1;
      case 5: { const k = Math.floor(t * 6); const r = Math.sin(k * 12.9898) * 43758.5; return (r - Math.floor(r)) * 2 - 1; }
      default: return 0;
    }
  };
  const N = 96, cycles = 2;
  const pts = [];
  for (let i = 0; i <= N; i++) {
    const t = (i / N) * cycles;
    pts.push(((i / N) * W).toFixed(1) + "," + (cy - shape(t) * amp).toFixed(1));
  }
  const phx = ((ph * cycles) % cycles) / cycles * W;
  const phy = cy - shape(ph * cycles) * amp;
  return (
    <svg className="viz scope" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <line x1="0" y1={cy} x2={W} y2={cy} className="mid" />
      <polyline points={pts.join(" ")} />
      <circle cx={phx.toFixed(1)} cy={phy.toFixed(1)} r="2" className="scope-dot" />
    </svg>
  );
}

function FilterCurve() {
  const [cut] = B.useSlider("cutoff");
  const [res] = B.useSlider("resonance");
  const [mode] = B.useChoice("filterMode");
  const W = 180, H = 46, N = 60;
  const fc = 0.06 + cut * 0.88;            // corner position (0..1 across the view)
  const q = 0.5 + res * 7;                  // resonance peak height
  const lp = mode === 0 || mode === 1, hp = mode === 2, bp = mode === 3, notch = mode === 4;
  const mag = (x) => {
    const d = (x - fc);
    const bump = res > 0.01 ? (q * 0.16) * Math.exp(-(d * d) / 0.0016) : 0;
    let base;
    if (lp) base = 1 / (1 + Math.pow(Math.max(0, (x - fc)) / 0.16, 2));
    else if (hp) base = 1 / (1 + Math.pow(Math.max(0, (fc - x)) / 0.16, 2));
    else if (bp) base = Math.exp(-(d * d) / 0.01);
    else if (notch) base = 1 - Math.exp(-(d * d) / 0.0009) * 0.95;
    else base = 0.5;
    return Math.max(0, Math.min(1.15, base + bump));
  };
  const pts = [];
  for (let i = 0; i <= N; i++) { const x = i / N; pts.push(((x) * W).toFixed(1) + "," + (H - 3 - mag(x) * (H - 8)).toFixed(1)); }
  return (
    <svg className="viz filter" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <polyline points={pts.join(" ")} />
      <line x1={fc * W} y1="0" x2={fc * W} y2={H} className="mark" />
    </svg>
  );
}

/* ============================ panels ============================ */
function OscPanel({ n, accent }) {
  const p = "osc" + n;
  return (
    <Panel title={"OSC " + n} accent={accent}>
      <div className="row">
        <Switch id={p + "On"} label="ON" />
        <Seg id={p + "Foot"} options={FOOTAGE} />
      </div>
      <div className="knobs">
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
      <div className="row">
        <Sel id={p + "Wave"} options={LFO_WAVES} />
        <Switch id={p + "Sync"} label="SYNC" />
        <div className="lfo-led" style={{ "--g": (lv * 0.5 + 0.5).toFixed(3) }} />
      </div>
      <div className="knobs">
        {sync ? <Sel id={p + "Div"} options={DIVISIONS} label="DIV" />
              : <Knob id={p + "Rate"} label="RATE" />}
        <Knob id={p + "Depth"} label="DEPTH" />
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
    <Panel title="ARPEGGIATOR" accent="#e07a9a">
      <div className="row">
        <Switch id="arpOn" label="ON" />
        <Switch id="arpLatch" label="LATCH" />
      </div>
      <div className={"row" + (on ? "" : " dim")}>
        <Sel id="arpMode" options={ARP_MODES} label="MODE" />
        <Sel id="arpSyncDiv" options={DIVISIONS} label="DIV" />
      </div>
      <div className={"knobs" + (on ? "" : " dim")}>
        <Knob id="arpGate" label="GATE" />
        <Knob id="arpSwing" label="SWING" />
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
        <IntPick id="maxVoices" label="MAX POLY" min={1} max={8} />
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
  const n = Math.round(1 + mv * 7); // maxVoices 1..8
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
      {browse && <Browser onClose={() => setBrowse(false)} />}
      {saving && <SaveDialog onClose={() => setSaving(false)} cat={preset.category} />}
    </header>
  );
}

function Browser({ onClose }) {
  const [list, setList] = useState([]);
  const [cat, setCat] = useState(-1);
  useEffect(() => { B.nativeFn("listPresets")().then((r) => setList(r || [])); }, []);
  const load = B.nativeFn("loadPreset");
  const shown = list.filter((e) => cat < 0 || e.category === cat);
  return (
    <div className="modal" onClick={onClose}>
      <div className="sheet" onClick={(e) => e.stopPropagation()}>
        <div className="sheet-h"><b>PRESETS</b><button onClick={onClose}>{"✕"}</button></div>
        <div className="cats">
          <button className={cat < 0 ? "on" : ""} onClick={() => setCat(-1)}>ALL</button>
          {CATEGORIES.map((c, i) =>
            <button key={i} className={cat === i ? "on" : ""} onClick={() => setCat(i)}>{c}</button>)}
        </div>
        <div className="plist">
          {shown.map((e) =>
            <button key={e.index} className={"pitem" + (e.factory ? "" : " user")}
                    onClick={() => { load(e.index); }}>
              <span className="pi-cat">{CATEGORIES[e.category]}</span>
              <span className="pi-nm">{e.name}</span>
              {!e.factory && <span className="pi-tag">USER</span>}
            </button>)}
        </div>
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
        <select className="sel" value={c} onChange={(e) => setC(+e.target.value)}>
          {CATEGORIES.map((x, i) => <option key={i} value={i}>{x}</option>)}
        </select>
        <button className="tbtn save" disabled={!name.trim()}
                onClick={() => { save(name.trim(), c); onClose(); }}>SAVE</button>
      </div>
    </div>
  );
}

/* ============================ app ============================ */
function App() {
  const modMap = useModMap();
  const meters = B.useEvent("meters", { lfo: [0, 0, 0], modSrc: [] });
  const modCtx = useMemo(() => ({ map: modMap, lfo: meters.lfo || [0, 0, 0], src: meters.modSrc || [] }),
                         [modMap, meters.lfo, meters.modSrc]);
  return (
    <ModContext.Provider value={modCtx}>
    <div id="app">
      <TopBar />
      <div className="cols">
        <div className="col">
          <OscPanel n={1} accent="#e07a7a" />
          <OscPanel n={2} accent="#e0a05a" />
          <OscPanel n={3} accent="#e0d05a" />
          <OscPanel n={4} accent="#c8d05a" />
          <OscPanel n={5} accent="#9ad05a" />
        </div>
        <div className="col">
          <MixerPanel />
          <VoicingPanel />
          <FilterPanel />
        </div>
        <div className="col">
          <EnvPanel n={1} name="AMP ENV" accent="#7ad0a0" />
          <EnvPanel n={2} name="FILTER ENV" accent="#5ad0e0" />
          <EnvPanel n={3} name="AUX ENV" accent="#5a9ae0" />
          <MasterPanel />
          <ReverbPanel />
        </div>
        <div className="col">
          <LfoPanel n={1} accent="#c08ae0" />
          <LfoPanel n={2} accent="#b07ae0" />
          <LfoPanel n={3} accent="#a06ae0" />
          <ArpPanel />
        </div>
        <div className="col">
          <ModMatrix />
          <FxPanel />
          <DelayPanel />
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
