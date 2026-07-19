#!/usr/bin/env bash
# packaging/macos/build-zip.sh — bundle the macOS build artefacts into an
# unsigned .zip.
#
# Produces:  $OUTPUT_ZIP    a zip of .component + .vst3 + .app with an
#                           INSTALL.txt explaining the one-time xattr -cr
#                           step (Gatekeeper quarantines unsigned bundles
#                           as "untrusted publisher").
#
# Inputs (env vars):
#   JOVE_PKG_VERSION  e.g. 1.1.0  (required; baked into the folder name)
#   ARTEFACT_DIR      path to JUCE's Jove_artefacts/Release/
#   OUTPUT_ZIP        destination .zip path
#
# UNSIGNED-RELEASE: stopgap until Developer ID signing + notarization are
# provisioned for this repo; the signed .pkg path can be copied from
# Doobie's packaging/macos/build-pkg.sh when that happens.

set -euo pipefail

: "${JOVE_PKG_VERSION:?JOVE_PKG_VERSION must be set}"
: "${ARTEFACT_DIR:?ARTEFACT_DIR must be set}"
: "${OUTPUT_ZIP:?OUTPUT_ZIP must be set}"

AU_BUNDLE="$ARTEFACT_DIR/AU/Jove.component"
VST3_BUNDLE="$ARTEFACT_DIR/VST3/Jove.vst3"
APP_BUNDLE="$ARTEFACT_DIR/Standalone/Jove.app"

for b in "$AU_BUNDLE" "$VST3_BUNDLE" "$APP_BUNDLE"; do
    if [[ ! -d "$b" ]]; then
        echo "missing artefact: $b" >&2
        exit 1
    fi
done

WORK="$(mktemp -d -t jove-zip-XXXX)"
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/Jove-$JOVE_PKG_VERSION-macOS-unsigned"
mkdir -p "$STAGE"

echo "==> copy bundles"
cp -R "$AU_BUNDLE"   "$STAGE/"
cp -R "$VST3_BUNDLE" "$STAGE/"
cp -R "$APP_BUNDLE"  "$STAGE/"

cat > "$STAGE/INSTALL.txt" <<'EOF'
Jove — unsigned macOS build
===========================

This build is NOT code-signed or notarized. macOS will quarantine it as
"untrusted publisher" until you strip the quarantine attribute. One
command per bundle after install does it:

  xattr -cr ~/Library/Audio/Plug-Ins/Components/Jove.component
  xattr -cr ~/Library/Audio/Plug-Ins/VST3/Jove.vst3
  xattr -cr /Applications/Jove.app

Install steps:

  1. Drop  Jove.component  →  ~/Library/Audio/Plug-Ins/Components
                            or /Library/Audio/Plug-Ins/Components
  2. Drop  Jove.vst3       →  ~/Library/Audio/Plug-Ins/VST3
                            or /Library/Audio/Plug-Ins/VST3
  3. Drop  Jove.app        →  /Applications  (or anywhere)
  4. Run the xattr -cr commands above (paths adjusted to where you
     actually installed the bundles).

Signed + notarized releases are planned; this is the interim state.
Alternatively, build from source — see README.md.
EOF

echo "==> zip"
mkdir -p "$(dirname "$OUTPUT_ZIP")"
# ditto preserves macOS bundle metadata + symlinks correctly across zip
# round-trips (plain `zip` mangles framework symlinks inside .app).
ditto -c -k --sequesterRsrc --keepParent "$STAGE" "$OUTPUT_ZIP"

echo ""
echo "Built unsigned macOS zip:"
echo "  $OUTPUT_ZIP"
