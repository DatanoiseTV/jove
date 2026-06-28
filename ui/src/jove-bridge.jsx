/* ============================================================
   Jove · JUCE <-> React bridge (core hooks only)
   Two-way binds React controls to APVTS parameters through the
   JUCE 8 WebSliderRelay / WebToggleButtonRelay / WebComboBoxRelay
   APIs, and receives the editor's 30 Hz native events.
   ============================================================ */
(function (global) {
  const { useState, useEffect, useCallback } = React;

  // Float param: relay publishes a normalised 0..1; JUCE maps to real range.
  function useSlider(id) {
    const relay = global.Juce.getSliderState(id);
    const [v, setV] = useState(relay.getNormalisedValue());
    useEffect(() => {
      const cb = () => setV(relay.getNormalisedValue());
      relay.valueChangedEvent.addListener(cb);
      return undefined;
    }, [id]);
    const set = useCallback((nv) => {
      const c = Math.max(0, Math.min(1, nv));
      relay.setNormalisedValue(c); setV(c);
    }, [id]);
    // scaledValue: the real engineering value (Hz, st, s, ...) for labels.
    const scaled = () => { try { return relay.getScaledValue(); } catch (_) { return v; } };
    return [v, set, scaled];
  }

  function useToggle(id) {
    const relay = global.Juce.getToggleState(id);
    const [v, setV] = useState(!!relay.getValue());
    useEffect(() => {
      const cb = () => setV(!!relay.getValue());
      relay.valueChangedEvent.addListener(cb);
      return undefined;
    }, [id]);
    const set = useCallback((b) => { relay.setValue(!!b); setV(!!b); }, [id]);
    return [v, set];
  }

  // Choice param: stored as 0..N-1; the relay also carries the option labels.
  function useChoice(id) {
    const relay = global.Juce.getComboBoxState(id);
    const [idx, setIdx] = useState(relay.getChoiceIndex());
    useEffect(() => {
      const cb = () => setIdx(relay.getChoiceIndex());
      relay.valueChangedEvent.addListener(cb);
      return undefined;
    }, [id]);
    const options = (relay.properties && relay.properties.choices) || [];
    const set = useCallback((i) => { relay.setChoiceIndex(i); setIdx(i); }, [id]);
    return [idx, set, options];
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
