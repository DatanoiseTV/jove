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
  const D = big ? 64 : 52, c = D / 2, sw = big ? 4 : 3.2, R = c - sw - 1;
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
  return (
    <div className={"knob" + (big ? " big" : "")} onMouseDown={down} onTouchStart={down}
         onWheel={wheel} onDoubleClick={() => set(bipolar ? 0.5 : 0)} title={label}>
      <svg width={D} height={D} viewBox={`0 0 ${D} ${D}`}>
        <path d={arc(A0, A0 + SWEEP)} className="k-track" strokeWidth={sw} />
        <path d={arc(Math.min(fillFrom, ang), Math.max(fillFrom, ang))} className="k-fill" strokeWidth={sw} />
        <circle cx={c} cy={c} r={R - sw - 1.5} className="k-body" />
        <line x1={c} y1={c} x2={px} y2={py} className="k-ptr" />
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

function Panel({ title, accent, children, wide, tall }) {
  return (
    <section className={"panel" + (wide ? " wide" : "") + (tall ? " tall" : "")}>
      <header className="ph" style={accent ? { "--accent": accent } : null}><span className="dot" />{title}</header>
      <div className="pbody">{children}</div>
    </section>
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
      <Seg id="filterMode" options={FILTER_MODES} label="MODE" />
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
  return (
    <div className={"modrow" + (src > 0 ? " active" : "")}>
      <span className="mn">{n}</span>
      <Sel id={p + "Src"} options={MOD_SRC} />
      <span className="arr">{"→"}</span>
      <Sel id={p + "Dst"} options={MOD_DST} />
      <Knob id={p + "Amt"} label="" bipolar />
    </div>
  );
}

function ModMatrix() {
  return (
    <Panel title="MOD MATRIX" accent="#c08ae0" wide tall>
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
        <Seg id="arpOctaves" options={["1", "2", "3", "4"]} label="OCT" />
        <Seg id="arpRatchet" options={["1", "2", "3", "4"]} label="RATCH" />
      </div>
    </Panel>
  );
}

function FxPanel() {
  return (
    <Panel title="FX" accent="#8ae0a8" wide>
      <div className="knobs">
        <Knob id="fxDrive" label="DRIVE" />
        <Knob id="fxChorus" label="CHORUS" />
        <Knob id="fxPhaser" label="PHASER" />
        <Knob id="fxDelay" label="DELAY" />
        <Knob id="fxReverb" label="REVERB" />
      </div>
      <Seg id="chorusMode" options={CHORUS_MODES} label="CHORUS MODE" />
    </Panel>
  );
}

function VoicingPanel() {
  const [mode] = B.useChoice("voiceMode");
  return (
    <Panel title="VOICING" accent="#9aa6e0">
      <Seg id="voiceMode" options={VOICE_MODES} label="MODE" />
      <div className="knobs">
        <Knob id="glideTime" label="GLIDE" />
        <Knob id="drift" label="DRIFT" />
        {mode === 2 && <Knob id="unisonDetune" label="UNI DET" />}
        {mode === 2 && <Knob id="unisonSpread" label="UNI SPR" />}
      </div>
      <div className="row">
        <Seg id="glideMode" options={GLIDE_MODES} label="GLIDE" />
        {mode === 2 && <Seg id="unisonCount" options={["1","2","3","4","5","6","7"]} label="VOICES" />}
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
        <Seg id="maxVoices" options={["1","2","3","4","5","6","7","8"]} label="POLY" />
      </div>
      <div className="tb-voices"><span className="v">{meters.voices}</span><span className="vl">VOICES</span></div>
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
  return (
    <div id="app">
      <TopBar />
      <div className="grid">
        <OscPanel n={1} accent="#e07a7a" />
        <OscPanel n={2} accent="#e0a05a" />
        <OscPanel n={3} accent="#e0d05a" />
        <MixerPanel />
        <VoicingPanel />
        <FilterPanel />
        <EnvPanel n={1} name="AMP ENV" accent="#7ad0a0" />
        <EnvPanel n={2} name="FILTER ENV" accent="#5ad0e0" />
        <EnvPanel n={3} name="AUX ENV" accent="#5a9ae0" />
        <LfoPanel n={1} accent="#c08ae0" />
        <LfoPanel n={2} accent="#b07ae0" />
        <LfoPanel n={3} accent="#a06ae0" />
        <ModMatrix />
        <ArpPanel />
        <FxPanel />
        <MasterPanel />
      </div>
      <footer className="foot">
        <span>{window.JOVE_VERSION_STR || ""}</span>
        <span>DatanoiseTV</span>
      </footer>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
