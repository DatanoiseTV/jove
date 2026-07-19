/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// macOS keyboard-focus workaround for the WebView editor.
//
// A focused WKWebView swallows key events and does NOT pass them up to the host,
// so a DAW's computer-keyboard MIDI ("musical typing") stops working the moment
// you click into the plugin UI. There is no clean JUCE C++ API for this; the
// established fix is to forward the WKWebView's keyDown:/keyUp: to the next
// responder (which reaches the host). We do it per-instance (isa-swizzle, not a
// process-wide class swizzle that could disturb other plugins) and gate it on
// whether an HTML text field is being edited, so typing preset names still works.
namespace jove
{
#if JUCE_MAC
// Install key-forwarding on the WKWebView found under `editorTopLevel`'s peer.
// Returns true once the native view was found and hooked (call again until it
// succeeds — the WKWebView may not exist on the first paint).
bool installHostKeyForwarding(juce::Component& editorTopLevel);

// The UI calls this (via a native function) when a text input gains/loses focus.
// While true, keys go to the WebView (typing); while false, they forward to the
// host (musical typing).
void setUiTextEditing(bool editing);

// Stop forwarding (keys fall back to the WebView's default handling). Call on
// editor teardown so a late key event can't touch freed state.
void disableHostKeyForwarding();
#else
// Non-mac: header-only no-op stubs. The implementation file is ObjC++
// (WebKeyboardFocus_mac.mm) and is only compiled on APPLE, so the stubs MUST
// live here — an unimplemented declaration links fine on macOS and then fails
// only on the Linux CI leg (undefined reference at Jove.so link).
inline bool installHostKeyForwarding(juce::Component&) { return true; }
inline void setUiTextEditing(bool) {}
inline void disableHostKeyForwarding() {}
#endif
} // namespace jove
