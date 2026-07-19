## macOS build is currently unsigned

The macOS build ships as an unsigned `.zip` for now (signed + notarized
installers are planned). Drop the bundles into:

  ~/Library/Audio/Plug-Ins/Components/Jove.component
  ~/Library/Audio/Plug-Ins/VST3/Jove.vst3
  /Applications/Jove.app   (or anywhere)

Then strip the Gatekeeper quarantine attribute once per bundle:

```
xattr -cr ~/Library/Audio/Plug-Ins/Components/Jove.component
xattr -cr ~/Library/Audio/Plug-Ins/VST3/Jove.vst3
xattr -cr /Applications/Jove.app
```

Or build from source (see README.md). Linux is unaffected (no signing
concept).

---
