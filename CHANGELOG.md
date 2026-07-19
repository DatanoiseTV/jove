# Changelog

All notable changes to Jove are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions follow
[semver](https://semver.org/).

## [1.1.1] - 2026-07-19

### Added
- CI (macOS + Linux build/test matrix) and a release pipeline: rolling
  nightly prerelease from `main`, versioned releases from tags, with
  downloadable macOS (universal, unsigned zip) and Linux artifacts

### Fixed
- Linux build: ObjC++ keyboard-forwarding glue is macOS-only now; non-mac
  builds use header-only stubs (undefined references at link, caught by CI)

## [1.1.0] - 2026-07-07

### Added
- `postGain` parameter: per-preset loudness trim (post-FX, pre-limiter,
  -26..+18 dB), exposed as the TRIM knob in the MASTER panel

### Changed
- Every factory preset's peak momentary loudness (ITU-R BS.1770
  K-weighted, 400 ms window, measured over a long hold so slow-attack
  pads are judged at the loudness they actually reach) is level-matched
  to the bank median. Spread reduced from 39.2 dB to 4.2 dB; 139/140
  presets within +/-1 dB. Boosts are capped so the pre-limiter true peak
  stays under -1 dBFS.

## [1.0.0] - 2026-07-07

First public release.

### Added
- 5-oscillator voice engine: BLEP morph + 64-table wavetable modes, sub,
  noise, ring mod, FM, hard/soft sync, unison, MPE
- Dual filters (single/serial/parallel): Moog-style ladder, SVF family,
  low-pass gate, Steiner-Parker — brightness-matched across models
- 3 envelopes, 3 LFOs, 32-slot modulation matrix with live-modulation UI,
  4 step sequencers
- EMS-style audio patchbay rewiring the voice topology (11×11 bipolar matrix)
- FX chain: 5-model drive with loudness-neutral makeup, multiband saturation,
  chorus/ensemble, phaser, synced delay (digital/BBD/tape), reverb
- Arpeggiator with 12 modes, ratcheting, swing, latch, transport lock
- 140 factory presets (12 categories), level-matched per category
- MIDI program change + host program list select the factory bank
- A/B compare with copy; DICE/VARY patch randomizer; revert-to-preset;
  unsaved-edits indicator; user-preset delete in the browser
- Audition mode: tempo-matched demo phrases per preset category
- Eco/HQ/Ultra quality (1×/2×/4× oversampling)
- React WebView UI (1500×1000 design canvas, aspect-locked resize), computer
  keyboard forwarded to the host so DAW musical typing keeps working

[1.0.0]: https://github.com/DatanoiseTV/jove/releases/tag/v1.0.0
