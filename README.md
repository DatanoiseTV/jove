# Jove

Analog-inspired polysynth instrument plugin built with JUCE 8. Ships as AU,
VST3, and a Standalone app on macOS. The UI is a React front end rendered in a
WebView and two-way bound to every parameter via JUCE's WebView relays.

## Layout

- `src/engine/` — JUCE-free DSP core (oscillators, filters, envelopes, voice
  allocation, effects). No JUCE dependency, so it builds and tests headlessly.
- `src/` — JUCE plugin processor, parameters, preset manager, WebView editor.
- `ui/` — React UI (`src/`) plus vendored runtime (`vendor/`), embedded into the
  binary as BinaryData at build time.
- `tests/` — engine/preset audit and APVTS round-trip checks.
- `external/JUCE` — JUCE, pinned as a git submodule.

## Build

Requires CMake 3.22+ and a C++20 toolchain.

```sh
git clone --recurse-submodules <repo-url> jove
cd jove
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

If you cloned without `--recurse-submodules`:

```sh
git submodule update --init --recursive
```

For fast local iteration on Apple silicon, add
`-DCMAKE_OSX_ARCHITECTURES=arm64`. CI builds should pass
`-DJOVE_INSTALL_LOCAL=OFF` so the build does not copy bundles into
`~/Library/Audio/Plug-Ins`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## License

GPL-3.0-or-later. See the license headers in the source files.
