/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "WebKeyboardFocus.h"

#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include <atomic>

// The objc runtime getters (object_getClass, ...) are annotated _Nullable but
// are non-null for a live view/class here; silence the benign nullable->nonnull
// conversions rather than litter the glue with casts.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullable-to-nonnull-conversion"

// ---------------------------------------------------------------------------
// Make the host's computer-MIDI keyboard work while a Jove editor is focused.
//
// A focused WKWebView swallows key events, so the DAW never sees them. We must
// redirect keys up the responder chain (which reaches the host) UNLESS the user
// is typing into a WebView text field.
//
// We deliberately do NOT subclass the WKWebView (no objc_allocateClassPair /
// object_setClass). A runtime-created subclass breaks macOS 26's NSDynamicProperty
// / KVO machinery: on window move or close JUCE reparents the WebView, WebKit's
// internal KVO fires `_viewDidChangeEffectiveCornerRadii`, and the computed-property
// descriptor lookup asserts on the synthetic class -> the whole host aborts
// (SIGABRT in NSDP_getComputedPropertyValue). Instead we install one app-local
// NSEvent key monitor and redirect only for the WebViews we tag, leaving every
// instance's real isa untouched.
// ---------------------------------------------------------------------------

namespace jove
{
namespace
{
// A WebView text field is focused -> keys must reach the WebView (typing).
std::atomic<bool> gTextEditing { false };

// Tags the WKWebViews Jove owns, so the shared monitor only ever redirects keys
// for our own views — never another plugin's WebView. Address used as the key.
const void* kJoveMarkKey = &kJoveMarkKey;

id  gMonitor      = nil; // single app-local key monitor shared by all editors
int gEditorCount  = 0;   // ref-count: remove the monitor when the last editor closes

NSView* findWKWebView(NSView* v)
{
    Class wk = objc_getClass("WKWebView");
    if (wk != nullptr && [v isKindOfClass: wk])
        return v;
    for (NSView* sub in [v subviews])
        if (NSView* found = findWKWebView(sub))
            return found;
    return nullptr;
}

// One of our tagged WKWebViews in the responder chain of `responder`? When web
// content is focused the first responder is an internal WKContentView subview,
// so we walk up nextResponder until we hit our tagged WKWebView (or nil).
NSView* markedWebViewInChain(NSResponder* responder)
{
    Class wk = objc_getClass("WKWebView");
    for (NSResponder* r = responder; r != nil; r = [r nextResponder])
        if (wk != nullptr && [r isKindOfClass: wk]
            && objc_getAssociatedObject(r, kJoveMarkKey) != nil)
            return (NSView*) r;
    return nullptr;
}
} // namespace

bool installHostKeyForwarding(juce::Component& editorTopLevel)
{
    auto* peer = editorTopLevel.getPeer();
    if (peer == nullptr)
        return false;

    NSView* root = (NSView*) peer->getNativeHandle();
    if (root == nullptr)
        return false;

    NSView* wk = findWKWebView(root);
    if (wk == nullptr)
        return false; // not realised yet — caller retries

    if (objc_getAssociatedObject(wk, kJoveMarkKey) != nil)
        return true;  // already managed

    objc_setAssociatedObject(wk, kJoveMarkKey, wk, OBJC_ASSOCIATION_ASSIGN);
    ++gEditorCount;

    if (gMonitor == nil)
    {
        gMonitor = [NSEvent addLocalMonitorForEventsMatchingMask: (NSEventMaskKeyDown | NSEventMaskKeyUp)
                    handler: ^NSEvent* (NSEvent* ev)
        {
            if (gTextEditing.load(std::memory_order_relaxed))
                return ev;                       // typing into a WebView field — leave it

            NSWindow* w = [ev window];
            if (w == nil)
                return ev;

            NSView* mk = markedWebViewInChain([w firstResponder]);
            if (mk == nil)
                return ev;                       // not aimed at one of our WebViews

            if (NSResponder* nr = [mk nextResponder])
            {
                if ([ev type] == NSEventTypeKeyDown) [nr keyDown: ev];
                else                                 [nr keyUp:   ev];
                return nil;                      // consumed: host got it, WebView won't
            }
            return ev;
        }];
    }
    return true;
}

void setUiTextEditing(bool editing)
{
    gTextEditing.store(editing, std::memory_order_relaxed);
}

void disableHostKeyForwarding()
{
    if (gEditorCount > 0)
        --gEditorCount;

    if (gEditorCount == 0 && gMonitor != nil)
    {
        [NSEvent removeMonitor: gMonitor];
        gMonitor = nil;
    }
}
} // namespace jove

#pragma clang diagnostic pop

#endif // JUCE_MAC — non-mac stubs are header-only inlines in WebKeyboardFocus.h
       // (this file is only compiled on APPLE; see CMakeLists)
