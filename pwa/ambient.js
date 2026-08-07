/*
 * Nocturne ambient engine: generative Web Audio soundscape plus
 * audio-reactive canvas visuals. No samples, no network — everything
 * is synthesized locally and starts on load (or first gesture when the
 * browser blocks autoplay).
 */
(() => {
  'use strict';

  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  // ------------------------------------------------------------------
  // Audio engine
  // ------------------------------------------------------------------

  const engine = {
    ctx: null,
    master: null,
    analyser: null,
    freqData: null,
    started: false,
    muted: false,
    timers: [],
    silentMedia: null,
  };

  const IS_IOS =
    /iPhone|iPad|iPod/.test(navigator.userAgent) ||
    (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);

  // Tiny silent WAV used as a media-element loop. iOS classifies Web Audio
  // as "ambient" and mutes it with the ring/silent switch; playing any
  // media element promotes the audio session to "playback", which ignores
  // the switch.
  const SILENT_WAV =
    'data:audio/wav;base64,UklGRigAAABXQVZFZm10IBIAAAABAAEARKwAAIhYAQACABAAAABkYXRhAgAAAAEA';

  function ensurePlaybackSession() {
    if (!IS_IOS) return;
    if (navigator.audioSession) {
      try {
        navigator.audioSession.type = 'playback';
      } catch (err) {
        /* older Safari: fall through to the media-element route */
      }
    }
    if (!engine.silentMedia) {
      const media = new Audio(SILENT_WAV);
      media.loop = true;
      media.setAttribute('playsinline', '');
      engine.silentMedia = media;
    }
    engine.silentMedia.play().catch(() => {});
  }

  // A minor pentatonic across a few octaves — calm, no wrong notes.
  const PENTATONIC = [220.0, 261.63, 293.66, 329.63, 392.0, 440.0, 523.25, 587.33, 659.25];
  // Slow chord cycle (frequencies of chord tones, low register).
  const CHORDS = [
    [110.0, 164.81, 220.0, 261.63], // Am
    [87.31, 130.81, 174.61, 220.0], // F maj
    [65.41, 130.81, 196.0, 246.94], // C maj7
    [98.0, 146.83, 196.0, 293.66],  // G sus
  ];

  function makeReverbImpulse(ctx, seconds, decay) {
    const rate = ctx.sampleRate;
    const length = Math.floor(rate * seconds);
    const impulse = ctx.createBuffer(2, length, rate);
    for (let channel = 0; channel < 2; channel += 1) {
      const data = impulse.getChannelData(channel);
      for (let i = 0; i < length; i += 1) {
        data[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, decay);
      }
    }
    return impulse;
  }

  function makeNoiseBuffer(ctx, seconds) {
    const rate = ctx.sampleRate;
    const length = Math.floor(rate * seconds);
    const buffer = ctx.createBuffer(1, length, rate);
    const data = buffer.getChannelData(0);
    let last = 0;
    for (let i = 0; i < length; i += 1) {
      // Lightly lowpassed white noise ≈ soft wind.
      const white = Math.random() * 2 - 1;
      last = last * 0.96 + white * 0.04;
      data[i] = last * 3.2;
    }
    return buffer;
  }

  function scheduleRepeat(fn, minMs, maxMs) {
    let cancelled = false;
    const tick = () => {
      if (cancelled || !engine.started) return;
      fn();
      const delay = minMs + Math.random() * (maxMs - minMs);
      const id = setTimeout(tick, delay);
      engine.timers.push({ id, cancel: () => { cancelled = true; clearTimeout(id); } });
    };
    const id = setTimeout(tick, minMs * Math.random());
    engine.timers.push({ id, cancel: () => { cancelled = true; clearTimeout(id); } });
  }

  function startDrone(ctx, reverb) {
    const droneGain = ctx.createGain();
    droneGain.gain.value = 0.0;
    droneGain.gain.linearRampToValueAtTime(0.16, ctx.currentTime + 8);

    const filter = ctx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.value = 240;
    filter.Q.value = 0.4;

    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.02;
    const lfoGain = ctx.createGain();
    lfoGain.gain.value = 90;
    lfo.connect(lfoGain).connect(filter.frequency);
    lfo.start();

    [[55.0, 'sine', 0.6], [110.0, 'triangle', 0.35], [164.81, 'sine', 0.22]].forEach(([freq, type, level]) => {
      const osc = ctx.createOscillator();
      osc.type = type;
      osc.frequency.value = freq;
      osc.detune.value = (Math.random() - 0.5) * 6;
      const g = ctx.createGain();
      g.gain.value = level;
      osc.connect(g).connect(filter);
      osc.start();
    });

    filter.connect(droneGain);
    droneGain.connect(engine.master);
    droneGain.connect(reverb);
  }

  function playPadChord(ctx, reverb, tones) {
    const now = ctx.currentTime;
    const attack = 5.0;
    const hold = 9.0;
    const release = 7.0;
    tones.forEach((freq) => {
      const osc = ctx.createOscillator();
      osc.type = 'sine';
      osc.frequency.value = freq * 2;
      osc.detune.value = (Math.random() - 0.5) * 10;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.0001, now);
      g.gain.exponentialRampToValueAtTime(0.045, now + attack);
      g.gain.setValueAtTime(0.045, now + attack + hold);
      g.gain.exponentialRampToValueAtTime(0.0001, now + attack + hold + release);
      const pan = ctx.createStereoPanner ? ctx.createStereoPanner() : null;
      if (pan) {
        pan.pan.value = (Math.random() - 0.5) * 0.8;
        osc.connect(g).connect(pan);
        pan.connect(reverb);
        pan.connect(engine.master);
      } else {
        osc.connect(g).connect(reverb);
        g.connect(engine.master);
      }
      osc.start(now);
      osc.stop(now + attack + hold + release + 0.5);
    });
  }

  function startPads(ctx, reverb) {
    let index = 0;
    playPadChord(ctx, reverb, CHORDS[0]);
    scheduleRepeat(() => {
      index = (index + 1) % CHORDS.length;
      playPadChord(ctx, reverb, CHORDS[index]);
    }, 16000, 22000);
  }

  function playBell(ctx, reverb) {
    const now = ctx.currentTime;
    const freq = PENTATONIC[Math.floor(Math.random() * PENTATONIC.length)];
    const decay = 3.5 + Math.random() * 4;
    const level = 0.05 + Math.random() * 0.05;

    const partials = [[1, 1.0], [2.0, 0.35], [2.99, 0.12]];
    const bellOut = ctx.createGain();
    bellOut.gain.value = 1.0;
    partials.forEach(([ratio, amp]) => {
      const osc = ctx.createOscillator();
      osc.type = 'sine';
      osc.frequency.value = freq * ratio;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.0001, now);
      g.gain.exponentialRampToValueAtTime(level * amp, now + 0.02);
      g.gain.exponentialRampToValueAtTime(0.0001, now + decay);
      osc.connect(g).connect(bellOut);
      osc.start(now);
      osc.stop(now + decay + 0.3);
    });

    const pan = ctx.createStereoPanner ? ctx.createStereoPanner() : null;
    if (pan) {
      pan.pan.value = (Math.random() - 0.5) * 1.4;
      bellOut.connect(pan);
      pan.connect(reverb);
      pan.connect(engine.master);
    } else {
      bellOut.connect(reverb);
      bellOut.connect(engine.master);
    }
  }

  function startBells(ctx, reverb) {
    scheduleRepeat(() => playBell(ctx, reverb), 3500, 9500);
  }

  function startWind(ctx, reverb) {
    const src = ctx.createBufferSource();
    src.buffer = makeNoiseBuffer(ctx, 7);
    src.loop = true;
    const filter = ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = 650;
    filter.Q.value = 0.5;
    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.045;
    const lfoGain = ctx.createGain();
    lfoGain.gain.value = 260;
    lfo.connect(lfoGain).connect(filter.frequency);
    lfo.start();
    const g = ctx.createGain();
    g.gain.value = 0.0;
    g.gain.linearRampToValueAtTime(0.02, ctx.currentTime + 12);
    src.connect(filter).connect(g);
    g.connect(engine.master);
    g.connect(reverb);
    src.start();
  }

  function startAudio() {
    if (engine.started) return;
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) return;
    const ctx = new Ctx();
    engine.ctx = ctx;

    engine.master = ctx.createGain();
    engine.master.gain.value = 0.9;

    const limiter = ctx.createDynamicsCompressor();
    limiter.threshold.value = -18;
    limiter.knee.value = 18;
    limiter.ratio.value = 8;
    limiter.attack.value = 0.01;
    limiter.release.value = 0.4;

    engine.analyser = ctx.createAnalyser();
    engine.analyser.fftSize = 512;
    engine.analyser.smoothingTimeConstant = 0.85;
    engine.freqData = new Uint8Array(engine.analyser.frequencyBinCount);

    const reverb = ctx.createConvolver();
    reverb.buffer = makeReverbImpulse(ctx, 3.8, 2.6);
    const reverbGain = ctx.createGain();
    reverbGain.gain.value = 0.5;
    reverb.connect(reverbGain).connect(engine.master);

    engine.master.connect(limiter);
    limiter.connect(engine.analyser);
    engine.analyser.connect(ctx.destination);

    engine.started = true;
    startDrone(ctx, reverb);
    startPads(ctx, reverb);
    startBells(ctx, reverb);
    startWind(ctx, reverb);

    if (ctx.state === 'suspended') {
      ctx.resume().catch(() => {});
    }
  }

  function toggleMute() {
    if (!engine.started) {
      startAudio();
      ensurePlaybackSession();
      setSoundIcon(true);
      return;
    }
    if (engine.ctx.state !== 'running' && !engine.muted) {
      // Audio was blocked rather than muted: treat the tap as "turn on".
      ensurePlaybackSession();
      engine.ctx.resume().catch(() => {});
      setSoundIcon(true);
      return;
    }
    engine.muted = !engine.muted;
    if (engine.muted) {
      if (engine.silentMedia) engine.silentMedia.pause();
      engine.ctx.suspend().catch(() => {});
    } else {
      ensurePlaybackSession();
      engine.ctx.resume().catch(() => {});
    }
    setSoundIcon(!engine.muted);
  }

  function setSoundIcon(on) {
    const iconOn = document.getElementById('icon-sound-on');
    const iconOff = document.getElementById('icon-sound-off');
    if (iconOn) iconOn.hidden = !on;
    if (iconOff) iconOff.hidden = on;
  }

  // Attempt autoplay; fall back to first gesture anywhere on the page.
  function armAutostart() {
    startAudio();
    const kick = (event) => {
      // The sound toggle manages its own state; letting the global unlock
      // run first would race with it and turn the first tap into a mute.
      const target = event && event.target;
      if (target && target.closest && target.closest('#sound-toggle')) return;
      startAudio();
      ensurePlaybackSession();
      // 'interrupted' is a non-standard Safari state (phone call, Siri),
      // so compare against 'running' rather than 'suspended'.
      if (engine.ctx && engine.ctx.state !== 'running' && !engine.muted) {
        engine.ctx.resume().catch(() => {});
        // Classic WebKit unlock: play a one-sample buffer inside the gesture.
        try {
          const src = engine.ctx.createBufferSource();
          src.buffer = engine.ctx.createBuffer(1, 1, 22050);
          src.connect(engine.ctx.destination);
          src.start(0);
        } catch (err) {
          /* ignore */
        }
      }
    };
    // iOS Safari only treats touchend/click as activation for audio unlock;
    // keep pointerdown/keydown for other browsers.
    ['pointerdown', 'touchend', 'click', 'keydown'].forEach((eventName) => {
      window.addEventListener(eventName, kick, { passive: true });
    });
    document.addEventListener('visibilitychange', () => {
      if (!document.hidden && engine.ctx && engine.ctx.state !== 'running' && !engine.muted) {
        engine.ctx.resume().catch(() => {});
      }
    });
  }

  // ------------------------------------------------------------------
  // Visuals
  // ------------------------------------------------------------------

  const view = {
    canvas: null,
    g: null,
    w: 0,
    h: 0,
    dpr: 1,
    stars: [],
    t: 0,
    bass: 0,
    mid: 0,
    treble: 0,
    hue: 218,
  };

  function resize() {
    const { canvas } = view;
    view.dpr = Math.min(window.devicePixelRatio || 1, 2);
    view.w = window.innerWidth;
    view.h = window.innerHeight;
    canvas.width = Math.floor(view.w * view.dpr);
    canvas.height = Math.floor(view.h * view.dpr);
    view.g.setTransform(view.dpr, 0, 0, view.dpr, 0, 0);
  }

  function seedStars() {
    view.stars = [];
    const count = Math.floor((view.w * view.h) / 9000);
    for (let i = 0; i < count; i += 1) {
      view.stars.push({
        x: Math.random(),
        y: Math.random() * 0.85,
        r: 0.4 + Math.random() * 1.2,
        phase: Math.random() * Math.PI * 2,
        speed: 0.3 + Math.random() * 0.8,
      });
    }
  }

  function sampleAudio() {
    if (!engine.started || !engine.analyser || (engine.ctx && engine.ctx.state !== 'running')) {
      // Gentle idle breathing when audio is not running yet.
      const idle = (Math.sin(view.t * 0.4) + 1) * 0.5;
      view.bass += (idle * 0.35 - view.bass) * 0.02;
      view.mid += (idle * 0.22 - view.mid) * 0.02;
      view.treble += (idle * 0.12 - view.treble) * 0.02;
      return;
    }
    engine.analyser.getByteFrequencyData(engine.freqData);
    const bins = engine.freqData.length;
    const avg = (from, to) => {
      let sum = 0;
      for (let i = from; i < to; i += 1) sum += engine.freqData[i];
      return sum / ((to - from) * 255);
    };
    const bass = avg(1, Math.floor(bins * 0.08));
    const mid = avg(Math.floor(bins * 0.08), Math.floor(bins * 0.35));
    const treble = avg(Math.floor(bins * 0.35), Math.floor(bins * 0.8));
    view.bass += (bass - view.bass) * 0.06;
    view.mid += (mid - view.mid) * 0.06;
    view.treble += (treble - view.treble) * 0.08;
  }

  function drawBackground(g) {
    const grad = g.createLinearGradient(0, 0, 0, view.h);
    const hue = view.hue;
    grad.addColorStop(0, `hsl(${hue + 14}, 55%, ${4 + view.bass * 3}%)`);
    grad.addColorStop(0.55, `hsl(${hue}, 48%, ${7 + view.bass * 4}%)`);
    grad.addColorStop(1, `hsl(${hue - 16}, 52%, ${5 + view.mid * 3}%)`);
    g.fillStyle = grad;
    g.fillRect(0, 0, view.w, view.h);
  }

  function drawStars(g) {
    g.save();
    for (const star of view.stars) {
      const tw = 0.45 + 0.55 * Math.sin(view.t * star.speed + star.phase);
      const alpha = (0.14 + 0.5 * tw) * (0.5 + view.treble * 1.4);
      g.globalAlpha = Math.min(alpha, 0.8);
      g.fillStyle = '#dfe8ff';
      g.beginPath();
      g.arc(star.x * view.w, star.y * view.h, star.r, 0, Math.PI * 2);
      g.fill();
    }
    g.restore();
  }

  function drawRibbon(g, baseY, amp, wavelength, speed, hueShift, alpha, thickness) {
    const points = 60;
    g.save();
    g.globalCompositeOperation = 'lighter';
    const grad = g.createLinearGradient(0, 0, view.w, 0);
    grad.addColorStop(0, `hsla(${view.hue + hueShift - 20}, 80%, 62%, 0)`);
    grad.addColorStop(0.5, `hsla(${view.hue + hueShift}, 85%, 66%, ${alpha})`);
    grad.addColorStop(1, `hsla(${view.hue + hueShift + 20}, 80%, 62%, 0)`);
    g.strokeStyle = grad;
    g.lineWidth = thickness;
    g.lineCap = 'round';
    g.beginPath();
    for (let i = 0; i <= points; i += 1) {
      const x = (i / points) * view.w;
      const wobble =
        Math.sin((x / view.w) * Math.PI * 2 * wavelength + view.t * speed) *
        amp *
        (0.6 + view.mid * 1.6);
      const drift = Math.sin((x / view.w) * Math.PI * 2 * (wavelength * 0.37) - view.t * speed * 0.6) * amp * 0.5;
      const y = baseY + wobble + drift;
      if (i === 0) g.moveTo(x, y);
      else g.lineTo(x, y);
    }
    g.stroke();
    g.restore();
  }

  function drawOrb(g) {
    const cx = view.w * 0.5;
    const cy = view.h * 0.46;
    const breathe = Math.sin(view.t * 0.5) * 0.5 + 0.5;
    const radius = Math.min(view.w, view.h) * (0.10 + breathe * 0.012 + view.bass * 0.05);

    g.save();
    g.globalCompositeOperation = 'lighter';
    const glow = g.createRadialGradient(cx, cy, 0, cx, cy, radius * 3.2);
    glow.addColorStop(0, `hsla(${view.hue + 26}, 90%, 78%, ${0.30 + view.bass * 0.30})`);
    glow.addColorStop(0.35, `hsla(${view.hue + 10}, 85%, 64%, ${0.12 + view.mid * 0.12})`);
    glow.addColorStop(1, 'hsla(220, 80%, 50%, 0)');
    g.fillStyle = glow;
    g.beginPath();
    g.arc(cx, cy, radius * 3.2, 0, Math.PI * 2);
    g.fill();

    const core = g.createRadialGradient(cx, cy, 0, cx, cy, radius);
    core.addColorStop(0, `hsla(${view.hue + 34}, 100%, 92%, ${0.85 + view.bass * 0.15})`);
    core.addColorStop(0.75, `hsla(${view.hue + 18}, 92%, 74%, 0.45)`);
    core.addColorStop(1, `hsla(${view.hue}, 85%, 60%, 0)`);
    g.fillStyle = core;
    g.beginPath();
    g.arc(cx, cy, radius, 0, Math.PI * 2);
    g.fill();
    g.restore();
  }

  function frame() {
    view.t += reduceMotion ? 0.004 : 0.012;
    view.hue = 218 + Math.sin(view.t * 0.05) * 26;
    sampleAudio();

    const g = view.g;
    drawBackground(g);
    drawStars(g);
    const midY = view.h * 0.58;
    drawRibbon(g, midY - view.h * 0.06, view.h * 0.05, 1.6, 0.55, 30, 0.30, 44);
    drawRibbon(g, midY, view.h * 0.07, 1.1, 0.38, 0, 0.26, 60);
    drawRibbon(g, midY + view.h * 0.08, view.h * 0.06, 2.1, 0.30, -34, 0.22, 50);
    drawOrb(g);

    requestAnimationFrame(frame);
  }

  // ------------------------------------------------------------------
  // Boot
  // ------------------------------------------------------------------

  window.addEventListener('DOMContentLoaded', () => {
    view.canvas = document.getElementById('scene');
    if (!view.canvas) return;
    view.g = view.canvas.getContext('2d');
    resize();
    seedStars();
    window.addEventListener('resize', () => {
      resize();
      seedStars();
    });

    const soundToggle = document.getElementById('sound-toggle');
    if (soundToggle) {
      soundToggle.addEventListener('click', (event) => {
        event.stopPropagation();
        toggleMute();
      });
    }

    armAutostart();
    requestAnimationFrame(frame);
  });

  // Minimal public handle for diagnostics and future core integration.
  window.nocturneAmbient = {
    get running() {
      return !!(engine.ctx && engine.ctx.state === 'running');
    },
    get muted() {
      return engine.muted;
    },
    start: startAudio,
    toggleMute,
  };
})();
