# Jove MPE demo files

Four MPE (MIDI Polyphonic Expression) demo clips, MPE **Lower Zone**: master
channel 1, member channels 2+, one note per member channel with per-note pitch
bend (±48 st), channel pressure, and CC74 (timbre). Standard Type-1 SMF.

| File | What it shows | Route in Jove to hear it |
|---|---|---|
| `01_mpe_pitch_glides.mid` | A C-major triad where each note bends to a different pitch independently | Nothing — per-note bend → pitch is automatic when MPE is on |
| `02_mpe_pressure_swell.mid` | A held chord, each note's pressure swelling at a different rate | Route **MPE PRS → CUTOFF** (or **→ AMP**) in the mod matrix |
| `03_mpe_timbre_sweep.mid` | A run of notes each sweeping CC74 0→127 | Route **MPE TMB → CUTOFF** |
| `04_mpe_expressive_lead.mid` | A melody combining per-note vibrato bend + pressure swell + timbre opening | Route **MPE PRS → AMP** and **MPE TMB → CUTOFF** |

## In Jove

1. Turn **MPE** on in the VOICING panel (set MPE BEND to 48 to match the files).
2. For the pressure/timbre demos, add the matrix routes listed above (the
   MOD tab) — bend→pitch is the only expression wired automatically.

## In Ableton Live (11+)

Ableton's plain MIDI-clip import flattens channels, so to get true MPE either:

- Drop the `.mid` onto an **MPE-enabled** track and confirm the per-note
  expression imports (Live keeps the channel→note mapping for MPE tracks), or
- Play the file from a track whose **MIDI To** is routed to the Jove track with
  MPE enabled, so the member-channel data reaches the plugin per-note.

In a fully MPE-aware host (Bitwig, Cubase, Logic) these import as expressive
per-note clips directly.

## Regenerating

`python3 gen_mpe.py` (no dependencies) writes the four files to `mpe_out/`.
Edit `BEND_RANGE` / the demo functions to taste.
