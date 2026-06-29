#!/usr/bin/env python3
"""Generate MPE (MIDI Polyphonic Expression) demo files, Lower Zone.
Master channel = 1 (index 0). Member channels = 2..16 (index 1..15).
Per-note pitch bend (range 48 st to match Jove's default), channel pressure,
and CC74 (timbre). Dependency-free Type-1 SMF writer."""
import struct, os

PPQ = 480
BEND_RANGE = 48  # semitones (Jove MPE default)

def vlq(n):
    out = bytearray([n & 0x7F]); n >>= 7
    while n: out.insert(0, (n & 0x7F) | 0x80); n >>= 7
    return bytes(out)

def bend14(semitones):
    v = int(round(8192 + (semitones / BEND_RANGE) * 8191))
    return max(0, min(16383, v))

class Track:
    def __init__(self): self.ev = []  # (abs_tick, bytes)
    def add(self, t, b): self.ev.append((t, bytes(b)))
    # --- channel helpers (ch is 1-based MIDI channel) ---
    def note_on(self, t, ch, note, vel):  self.add(t, [0x90|(ch-1), note, vel])
    def note_off(self, t, ch, note):      self.add(t, [0x80|(ch-1), note, 0])
    def bend(self, t, ch, semis):
        v = bend14(semis); self.add(t, [0xE0|(ch-1), v & 0x7F, (v >> 7) & 0x7F])
    def pressure(self, t, ch, val):       self.add(t, [0xD0|(ch-1), val & 0x7F])
    def cc(self, t, ch, num, val):        self.add(t, [0xB0|(ch-1), num & 0x7F, val & 0x7F])
    def timbre(self, t, ch, val):         self.cc(t, ch, 74, val)
    # RPN helper (sets data entry MSB)
    def rpn(self, t, ch, msb, lsb, data):
        self.cc(t, ch, 101, msb); self.cc(t, ch, 100, lsb); self.cc(t, ch, 6, data)
    def meta_name(self, t, name):
        b = name.encode(); self.add(t, [0xFF, 0x03, len(b)] + list(b))
    def encode(self):
        self.ev.sort(key=lambda e: e[0])
        out = bytearray(); last = 0
        for t, b in self.ev:
            out += vlq(t - last) + b; last = t
        out += vlq(0) + bytes([0xFF, 0x2F, 0x00])
        return bytes(out)

def tempo_track(bpm=96):
    tr = Track(); uspq = int(60_000_000 / bpm)
    tr.add(0, [0xFF, 0x51, 0x03, (uspq>>16)&0xFF, (uspq>>8)&0xFF, uspq&0xFF])
    tr.add(0, [0xFF, 0x58, 0x04, 4, 2, 24, 8])  # 4/4
    return tr

def mpe_setup(tr, members=15):
    # MPE Configuration Message: RPN 6 on master channel = member channel count.
    tr.rpn(0, 1, 0x00, 0x06, members)
    # Per-note pitch bend range (RPN 0) on master + every member channel.
    for ch in range(1, 17):
        tr.rpn(0, ch, 0x00, 0x00, BEND_RANGE)

def write_smf(path, name, data_track):
    tempo = tempo_track().encode()
    data = data_track.encode()
    with open(path, 'wb') as f:
        f.write(b'MThd' + struct.pack('>IHHH', 6, 1, 2, PPQ))
        f.write(b'MTrk' + struct.pack('>I', len(tempo)) + tempo)
        f.write(b'MTrk' + struct.pack('>I', len(data)) + data)
    print("wrote", path, "(%d bytes)" % os.path.getsize(path))

def ramp(tr, ch, t0, t1, fn, emit, steps=48):
    """emit(t, ch, value 0..1) over [t0,t1) following fn(0..1)->0..1."""
    for i in range(steps):
        u = i / (steps - 1)
        emit(t0 + int((t1 - t0) * u), ch, fn(u))

OUT = os.path.join(os.path.dirname(__file__), 'mpe_out')
os.makedirs(OUT, exist_ok=True)

# ---- 1. Pitch glides: a C-major triad, each note bends independently ---------
def demo_pitch_glides():
    tr = Track(); tr.meta_name(0, "Jove MPE - Pitch Glides"); mpe_setup(tr)
    chord = [60, 64, 67]; targets = [+12, -2, +7]   # each note slides somewhere else
    dur = PPQ * 8
    for i, note in enumerate(chord):
        ch = 2 + i
        tr.note_on(0, ch, note, 96); tr.bend(0, ch, 0)
        ramp(tr, ch, PPQ, PPQ*7,
             lambda u, tg=targets[i]: u, lambda t, c, u, tg=targets[i]: tr.bend(t, c, tg*u))
        tr.bend(dur, ch, targets[i]); tr.note_off(dur, ch, note)
    write_smf(os.path.join(OUT, "01_mpe_pitch_glides.mid"), "glides", tr)

# ---- 2. Pressure swells: held chord, per-note pressure in/out (route MPE PRS) -
def demo_pressure_swell():
    tr = Track(); tr.meta_name(0, "Jove MPE - Pressure Swell"); mpe_setup(tr)
    chord = [48, 55, 60, 64]; dur = PPQ * 8
    import math
    for i, note in enumerate(chord):
        ch = 2 + i
        tr.note_on(0, ch, note, 64); tr.pressure(0, ch, 0)
        cycles = 1 + i  # each voice swells at a different rate
        ramp(tr, ch, 0, dur,
             lambda u, c=cycles: 0.5 - 0.5*math.cos(2*math.pi*c*u),
             lambda t, c, v: tr.pressure(t, c, int(v*127)), steps=96)
        tr.pressure(dur, ch, 0); tr.note_off(dur, ch, note)
    write_smf(os.path.join(OUT, "02_mpe_pressure_swell.mid"), "pressure", tr)

# ---- 3. Timbre sweep: notes with CC74 sweeps (route MPE TMB -> Cutoff) --------
def demo_timbre_sweep():
    tr = Track(); tr.meta_name(0, "Jove MPE - Timbre Sweep"); mpe_setup(tr)
    seq = [60, 62, 64, 65, 67, 69, 71, 72]; step = PPQ
    for i, note in enumerate(seq):
        ch = 2 + (i % 6)
        t = i * step
        tr.note_on(t, ch, note, 100); tr.timbre(t, ch, 0)
        ramp(tr, ch, t, t+step,
             lambda u: u, lambda tt, c, v: tr.timbre(tt, c, int(v*127)), steps=24)
        tr.timbre(t+step, ch, 127); tr.note_off(t+step, ch, note)
    write_smf(os.path.join(OUT, "03_mpe_timbre_sweep.mid"), "timbre", tr)

# ---- 4. Expressive lead: melody with combined bend + pressure + timbre --------
def demo_expressive_lead():
    tr = Track(); tr.meta_name(0, "Jove MPE - Expressive Lead"); mpe_setup(tr)
    import math
    melody = [(64,2),(67,1),(69,1),(72,2),(71,1),(67,1),(69,4)]  # (note, beats)
    t = 0
    for i, (note, beats) in enumerate(melody):
        ch = 2 + (i % 8); dur = int(PPQ * beats)
        tr.note_on(t, ch, note, 100)
        # expressive vibrato bend that grows over the note
        ramp(tr, ch, t, t+dur,
             lambda u: u,
             lambda tt, c, u, T=t, D=dur: tr.bend(tt, c, 0.4*math.sin(2*math.pi*5*((tt-T)/PPQ))*u),
             steps=max(24, beats*24))
        # pressure swell in
        ramp(tr, ch, t, t+dur, lambda u: min(1.0, u*1.5),
             lambda tt, c, v: tr.pressure(tt, c, int(v*127)), steps=32)
        # slow timbre open
        ramp(tr, ch, t, t+dur, lambda u: 0.3+0.7*u,
             lambda tt, c, v: tr.timbre(tt, c, int(v*127)), steps=24)
        tr.bend(t+dur, ch, 0); tr.note_off(t+dur, ch, note)
        t += dur
    write_smf(os.path.join(OUT, "04_mpe_expressive_lead.mid"), "lead", tr)

demo_pitch_glides(); demo_pressure_swell(); demo_timbre_sweep(); demo_expressive_lead()
print("done ->", OUT)
