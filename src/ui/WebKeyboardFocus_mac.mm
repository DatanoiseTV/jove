/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "WebKeyboardFocus.h"

#if JUCE_MAC

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <atomic>

// The objc runtime getters (object_getClass, class_getName, ...) are annotated
// _Nullable but are non-null for a live view/class here; silence the benign
// nullable->nonnull conversions rather than litter the glue with casts.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullable-to-nonnull-conversion"

namespace jove
{
namespace
{
// While a WebView text field is focused, keys must reach the WebView (typing);
// otherwise they forward to the host so its musical typing works.
std::atomic<bool> gTextEditing { false };
// Cleared on editor teardown so a stray key can't run the forwarding path.
std::atomic<bool> gForwardingActive { false };

// Call the instance's inherited (WKWebView default) implementation.
void callSuper(id self, SEL cmd, NSEvent* ev)
{
    struct objc_super sup = { self, class_getSuperclass(object_getClass(self)) };
    ((void (*)(struct objc_super*, SEL, NSEvent*)) objc_msgSendSuper)(&sup, cmd, ev);
}

void joveKeyDown(id self, SEL cmd, NSEvent* ev)
{
    if (! gForwardingActive.load(std::memory_order_relaxed) || gTextEditing.load(std::memory_order_relaxed))
        callSuper(self, cmd, ev);              // let the WebView handle it (typing)
    else if (NSResponder* nr = [(NSView*) self nextResponder])
        [nr keyDown: ev];                       // hand off up the chain -> host
}

void joveKeyUp(id self, SEL cmd, NSEvent* ev)
{
    if (! gForwardingActive.load(std::memory_order_relaxed) || gTextEditing.load(std::memory_order_relaxed))
        callSuper(self, cmd, ev);
    else if (NSResponder* nr = [(NSView*) self nextResponder])
        [nr keyUp: ev];
}

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

// Marker method: proves an instance is already our forwarding subclass, so a
// retried install never swizzles the same view twice.
BOOL joveIsForwarderImp(id, SEL) { return YES; }
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

    // Per-instance subclass (KVO-style isa-swizzle): keeps every other WKWebView
    // in the process — including other plugins' — completely untouched.
    Class base = object_getClass(wk);
    if (! class_respondsToSelector(base, @selector(joveIsForwarder)))
    {
        NSString* name = [NSString stringWithFormat: @"JoveKeyFwd_%s_%p", class_getName(base), (void*) wk];
        Class sub = objc_allocateClassPair(base, [name UTF8String], 0);
        if (sub == nullptr)
            return false;
        class_addMethod(sub, @selector(keyDown:), (IMP) joveKeyDown, "v@:@");
        class_addMethod(sub, @selector(keyUp:),   (IMP) joveKeyUp,   "v@:@");
        // marker so we never double-swizzle the same instance
        class_addMethod(sub, @selector(joveIsForwarder), (IMP) joveIsForwarderImp, "c@:");
        objc_registerClassPair(sub);
        object_setClass(wk, sub);
    }

    gForwardingActive.store(true, std::memory_order_relaxed);
    return true;
}

void setUiTextEditing(bool editing)
{
    gTextEditing.store(editing, std::memory_order_relaxed);
}

void disableHostKeyForwarding()
{
    gForwardingActive.store(false, std::memory_order_relaxed);
}
} // namespace jove

#pragma clang diagnostic pop

#else // ---- non-macOS: no WKWebView, nothing to do -------------------------

namespace jove
{
bool installHostKeyForwarding(juce::Component&) { return true; }
void setUiTextEditing(bool) {}
void disableHostKeyForwarding() {}
} // namespace jove

#endif
