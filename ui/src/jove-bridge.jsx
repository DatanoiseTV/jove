/* ============================================================
   Jove · JUCE <-> React bridge (core hooks only)
   Two-way binds React controls to APVTS parameters through the
   JUCE 8 WebSliderRelay / WebToggleButtonRelay / WebComboBoxRelay
   APIs, and receives the editor's 30 Hz native events.
   ============================================================ */
(function (global) {
  const { useState, useEffect, useCallback } = React;

  // Force a re-render. We read relay values directly during render (rather than
  // snapshotting into state) so the value is always current, and re-render on
  // BOTH value and properties changes — the initial value/range arrive async,
  // after first render, so a stale snapshot would leave controls reading 0.
  function useBind(relay) {
    const [, bump] = useState(0);
    useEffect(() => {
      const cb = () => bump((x) => (x + 1) | 0);
      relay.valueChangedEvent.addListener(cb);
      if (relay.propertiesChangedEvent) relay.propertiesChangedEvent.addListener(cb);
      cb(); // sync whatever arrived before this effect ran
      return undefined; // JUCE relays expose no removeListener; cached relay -> fine
    }, [relay]);
  }

  // Float param: relay publishes a normalised 0..1; JUCE maps to the real range.
  function useSlider(id) {
    const relay = global.Juce.getSliderState(id);
    useBind(relay);
    const set = useCallback((nv) => relay.setNormalisedValue(Math.max(0, Math.min(1, nv))), [relay]);
    const scaled = () => { try { return relay.getScaledValue(); } catch (_) { return relay.getNormalisedValue(); } };
    return [relay.getNormalisedValue(), set, scaled];
  }

  function useToggle(id) {
    const relay = global.Juce.getToggleState(id);
    useBind(relay);
    const set = useCallback((b) => relay.setValue(!!b), [relay]);
    return [!!relay.getValue(), set];
  }

  // Choice param: stored as 0..N-1; the relay also carries the option labels.
  function useChoice(id) {
    const relay = global.Juce.getComboBoxState(id);
    useBind(relay);
    const options = (relay.properties && relay.properties.choices) || [];
    const set = useCallback((i) => relay.setChoiceIndex(i), [relay]);
    return [relay.getChoiceIndex(), set, options];
  }

  // Native events emitted by the editor (meters, preset).
  function useEvent(name, initial) {
    const [v, setV] = useState(initial);
    useEffect(() => {
      const cb = (e) => setV(e);
      global.Juce.backend.addEventListener(name, cb);
      return () => global.Juce.backend.removeEventListener(name, cb);
    }, [name]);
    return v;
  }

  function nativeFn(name) {
    try { return global.Juce.getNativeFunction(name); }
    catch (_) { return () => Promise.resolve(); }
  }

  global.JoveBridge = { useSlider, useToggle, useChoice, useEvent, nativeFn };
})(window);
