const els = {};
const state = {
  username: '',
  token: '',
  repo: null,
  repoOwner: 'CastaliaInstitute',
  bleDevice: null,
};

const audio = {
  ctx: null,
  master: null,
  limiter: null,
  voices: [],
  playing: false,
  volume: 0.6,
};

function init() {
  els.loginForm = document.getElementById('login-form');
  els.username = document.getElementById('username');
  els.token = document.getElementById('token');
  els.authStatus = document.getElementById('auth-status');
  els.repoStatus = document.getElementById('repo-status');
  els.bleStatus = document.getElementById('ble-status');
  els.log = document.getElementById('log');

  els.discoverRepo = document.getElementById('discover-repo');
  els.openRepo = document.getElementById('open-repo');
  els.connectDevice = document.getElementById('connect-device');
  els.disconnectDevice = document.getElementById('disconnect-device');
  els.testOffline = document.getElementById('test-offline');
  els.registerSw = document.getElementById('register-sw');
  els.playAudio = document.getElementById('play-audio');
  els.stopAudio = document.getElementById('stop-audio');
  els.volume = document.getElementById('volume');
  els.audioStatus = document.getElementById('audio-status');

  els.loginForm.addEventListener('submit', onLogin);
  els.discoverRepo.addEventListener('click', onDiscoverRepo);
  els.openRepo.addEventListener('click', onOpenRepo);
  els.connectDevice.addEventListener('click', onConnectDevice);
  els.disconnectDevice.addEventListener('click', onDisconnectDevice);
  els.testOffline.addEventListener('click', onOfflineTest);
  els.registerSw.addEventListener('click', onRegisterServiceWorker);
  els.playAudio.addEventListener('click', onPlaySoundscape);
  els.stopAudio.addEventListener('click', onStopSoundscape);
  els.volume.addEventListener('input', onVolumeChange);
  document.addEventListener('visibilitychange', onVisibilityChange);

  loadSession();
}

function setStatus(node, message) {
  node.textContent = message;
}

function logLine(message) {
  const now = new Date().toISOString();
  els.log.textContent = `${now} ${message}\n${els.log.textContent}`;
}

function loadSession() {
  const raw = localStorage.getItem('nocturne-pwa-session');
  if (!raw) {
    return;
  }
  try {
    const session = JSON.parse(raw);
    state.username = session.username || '';
    state.token = session.token || '';
    if (state.username) {
      els.username.value = state.username;
      setAuthenticated();
      onLoginStateUpdated();
    }
  } catch (error) {
    logLine(`Failed to parse cached session: ${error.message}`);
  }
}

function saveSession() {
  localStorage.setItem(
    'nocturne-pwa-session',
    JSON.stringify({ username: state.username, token: state.token })
  );
}

function clearSession() {
  localStorage.removeItem('nocturne-pwa-session');
  state.username = '';
  state.token = '';
}

function setAuthenticated() {
  setStatus(els.authStatus, `Connected as ${state.username}`);
  els.discoverRepo.disabled = false;
  els.connectDevice.disabled = false;
}

function onLogin(event) {
  event.preventDefault();
  state.username = (els.username.value || '').trim();
  state.token = (els.token.value || '').trim();
  if (!state.username || !state.token) {
    setStatus(els.authStatus, 'Missing username or token.');
    return;
  }
  const clean = state.username.toLowerCase().replace(/^castalia-/, '');
  if (clean !== state.username) {
    state.username = clean;
    els.username.value = clean;
  }
  setAuthenticated();
  saveSession();
  onLoginStateUpdated();
}

function onLoginStateUpdated() {
  els.discoverRepo.disabled = false;
  els.connectDevice.disabled = false;
  setStatus(els.repoStatus, `Ready to resolve castalia-${state.username}`);
  setStatus(els.bleStatus, state.bleDevice ? `Connected: ${state.bleDevice.name}` : 'No connected device');
}

async function githubGet(url, token) {
  const response = await fetch(url, {
    headers: {
      Accept: 'application/vnd.github+json',
      Authorization: `Bearer ${token}`,
    },
  });
  if (!response.ok) {
    const body = await response.text();
    throw new Error(`${response.status} ${response.statusText}: ${body}`);
  }
  return response.json();
}

function repoNameForUser(username) {
  return `castalia-${username}`;
}

async function onDiscoverRepo() {
  if (!state.username || !state.token) {
    setStatus(els.repoStatus, 'Please sign in first');
    return;
  }
  els.discoverRepo.disabled = true;
  setStatus(els.repoStatus, 'Discovering repository...');
  const repoName = repoNameForUser(state.username);
  const repoSlug = `${state.repoOwner}/${repoName}`;
  try {
    state.repo = await githubGet(`https://api.github.com/repos/${repoSlug}`, state.token);
    els.openRepo.disabled = false;
    setStatus(els.repoStatus, `Found ${state.repo.full_name} (id ${state.repo.id})`);
    logLine(`Resolved repo: ${state.repo.full_name}`);
  } catch (error) {
    setStatus(els.repoStatus, `Repository not available: ${error.message}`);
    logLine(`Repo lookup failed for ${repoSlug}`);
  } finally {
    els.discoverRepo.disabled = false;
  }
}

function onOpenRepo() {
  if (!state.repo?.html_url) return;
  window.open(state.repo.html_url, '_blank', 'noopener,noreferrer');
}

async function onConnectDevice() {
  if (!('bluetooth' in navigator)) {
    setStatus(els.bleStatus, 'Web Bluetooth not supported');
    return;
  }

  try {
    const device = await navigator.bluetooth.requestDevice({
      acceptAllDevices: true,
      optionalServices: ['battery_service', 'heart_rate'],
    });
    state.bleDevice = device;
    state.bleDevice.addEventListener('gattserverdisconnected', () => {
      setStatus(els.bleStatus, `Disconnected: ${device.name}`);
      els.disconnectDevice.disabled = true;
      state.bleDevice = null;
      logLine(`Device disconnected: ${device.name}`);
    });
    const server = await device.gatt.connect();
    const services = await server.getPrimaryServices();
    setStatus(els.bleStatus, `Connected: ${device.name} (${services.length} service(s))`);
    els.disconnectDevice.disabled = false;
    logLine(`WebBLE connected: ${device.name}`);
  } catch (error) {
    setStatus(els.bleStatus, `Device connect failed: ${error.message}`);
  }
}

function onDisconnectDevice() {
  if (state.bleDevice?.gatt?.connected) {
    state.bleDevice.gatt.disconnect();
  }
  state.bleDevice = null;
  els.disconnectDevice.disabled = true;
  setStatus(els.bleStatus, 'No connected device');
}

function onOfflineTest() {
  const snapshot = {
    id: crypto.randomUUID(),
    ts: Date.now(),
    repo: state.repo ? state.repo.full_name : null,
    username: state.username || null,
    online: navigator.onLine,
    agent: navigator.userAgent,
  };
  localStorage.setItem('nocturne-offline-snapshot', JSON.stringify(snapshot));
  logLine(`Offline snapshot created: ${snapshot.id}`);
}

async function onRegisterServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    setStatus(els.repoStatus, 'Service workers not supported');
    return;
  }
  try {
    const registration = await navigator.serviceWorker.register('./service-worker.js', { scope: './' });
    setStatus(els.repoStatus, `Service worker active: ${registration.scope}`);
    logLine('Service worker registered');
  } catch (error) {
    setStatus(els.repoStatus, `SW registration failed: ${error.message}`);
    logLine(`SW registration failed: ${error.message}`);
  }
}

// --- Soundscape playback ---
// iOS Safari rules that shape this code:
// 1. An AudioContext may only start (or resume) inside a user gesture handler.
// 2. The context is created in the "suspended" state and must be resumed explicitly.
// 3. Backgrounding or a phone call moves the context to "interrupted"; it must be
//    resumed when the page becomes visible again.
// 4. The hardware silent switch mutes Web Audio unless the audio session is
//    promoted to "playback" (navigator.audioSession, iOS 16.4+).

function ensureAudioContext() {
  if (audio.ctx) {
    return audio.ctx;
  }
  const Ctx = window.AudioContext || window.webkitAudioContext;
  if (!Ctx) {
    return null;
  }
  audio.ctx = new Ctx();
  audio.ctx.addEventListener('statechange', () => {
    logLine(`Audio context state: ${audio.ctx.state}`);
    if (audio.playing && audio.ctx.state === 'running') {
      setStatus(els.audioStatus, 'Playing nocturne soundscape');
    }
  });
  return audio.ctx;
}

function configureAudioSession() {
  // Keeps playback audible with the iPhone silent switch on (iOS 16.4+).
  if ('audioSession' in navigator) {
    try {
      navigator.audioSession.type = 'playback';
    } catch (error) {
      logLine(`audioSession configuration failed: ${error.message}`);
    }
  }
}

function unlockAudio(ctx) {
  // Older iOS versions only unlock after a buffer actually plays in the gesture.
  const buffer = ctx.createBuffer(1, 1, ctx.sampleRate);
  const source = ctx.createBufferSource();
  source.buffer = buffer;
  source.connect(ctx.destination);
  source.start(0);
}

function createNoiseBuffer(ctx, seconds) {
  const length = Math.floor(ctx.sampleRate * seconds);
  const buffer = ctx.createBuffer(1, length, ctx.sampleRate);
  const data = buffer.getChannelData(0);
  let last = 0;
  for (let i = 0; i < length; i += 1) {
    const white = Math.random() * 2 - 1;
    last = (last + 0.02 * white) / 1.02;
    data[i] = last * 3.5;
  }
  return buffer;
}

function spawnDrone(ctx, destination, frequency, level, lfoRate) {
  const osc = ctx.createOscillator();
  osc.type = 'sine';
  osc.frequency.value = frequency;

  const gain = ctx.createGain();
  gain.gain.value = level;

  const lfo = ctx.createOscillator();
  lfo.frequency.value = lfoRate;
  const lfoDepth = ctx.createGain();
  lfoDepth.gain.value = level * 0.4;
  lfo.connect(lfoDepth);
  lfoDepth.connect(gain.gain);

  osc.connect(gain);
  gain.connect(destination);
  osc.start();
  lfo.start();
  audio.voices.push(osc, lfo);
}

function buildSoundscape(ctx) {
  audio.master = ctx.createGain();
  audio.master.gain.value = 0;

  // Soft safety limiter so layered voices can never clip the output.
  audio.limiter = ctx.createDynamicsCompressor();
  audio.limiter.threshold.value = -18;
  audio.limiter.knee.value = 12;
  audio.limiter.ratio.value = 12;
  audio.limiter.attack.value = 0.003;
  audio.limiter.release.value = 0.25;

  audio.master.connect(audio.limiter);
  audio.limiter.connect(ctx.destination);

  spawnDrone(ctx, audio.master, 55, 0.22, 0.05);
  spawnDrone(ctx, audio.master, 110.3, 0.14, 0.08);
  spawnDrone(ctx, audio.master, 164.8, 0.08, 0.11);
  spawnDrone(ctx, audio.master, 220.6, 0.05, 0.07);

  const noise = ctx.createBufferSource();
  noise.buffer = createNoiseBuffer(ctx, 4);
  noise.loop = true;
  const noiseFilter = ctx.createBiquadFilter();
  noiseFilter.type = 'lowpass';
  noiseFilter.frequency.value = 420;
  const noiseGain = ctx.createGain();
  noiseGain.gain.value = 0.12;
  noise.connect(noiseFilter);
  noiseFilter.connect(noiseGain);
  noiseGain.connect(audio.master);
  noise.start();
  audio.voices.push(noise);

  audio.master.gain.linearRampToValueAtTime(audio.volume, ctx.currentTime + 2.5);
}

function setupMediaSession() {
  if (!('mediaSession' in navigator)) {
    return;
  }
  navigator.mediaSession.metadata = new MediaMetadata({
    title: 'Nocturne Soundscape',
    artist: 'Nocturne',
    album: 'Adaptive Ambience',
  });
  navigator.mediaSession.setActionHandler('play', () => onPlaySoundscape());
  navigator.mediaSession.setActionHandler('pause', () => onStopSoundscape());
  navigator.mediaSession.setActionHandler('stop', () => onStopSoundscape());
}

async function onPlaySoundscape() {
  if (audio.playing) {
    return;
  }
  const ctx = ensureAudioContext();
  if (!ctx) {
    setStatus(els.audioStatus, 'Web Audio is not supported in this browser.');
    return;
  }
  try {
    configureAudioSession();
    if (ctx.state !== 'running') {
      await ctx.resume();
    }
    unlockAudio(ctx);
    buildSoundscape(ctx);
    setupMediaSession();
    audio.playing = true;
    els.playAudio.disabled = true;
    els.stopAudio.disabled = false;
    setStatus(els.audioStatus, 'Playing nocturne soundscape');
    logLine('Soundscape started');
  } catch (error) {
    setStatus(els.audioStatus, `Playback failed: ${error.message}`);
    logLine(`Playback failed: ${error.message}`);
  }
}

async function onStopSoundscape() {
  if (!audio.playing || !audio.ctx) {
    return;
  }
  const ctx = audio.ctx;
  audio.playing = false;
  audio.master.gain.cancelScheduledValues(ctx.currentTime);
  audio.master.gain.setValueAtTime(audio.master.gain.value, ctx.currentTime);
  audio.master.gain.linearRampToValueAtTime(0, ctx.currentTime + 0.8);
  const voices = audio.voices;
  audio.voices = [];
  setTimeout(() => {
    voices.forEach((node) => {
      try {
        node.stop();
      } catch (error) {
        // Node already stopped.
      }
    });
    audio.master?.disconnect();
    audio.limiter?.disconnect();
    audio.master = null;
    audio.limiter = null;
  }, 900);
  els.playAudio.disabled = false;
  els.stopAudio.disabled = true;
  setStatus(els.audioStatus, 'Stopped');
  logLine('Soundscape stopped');
}

function onVolumeChange() {
  audio.volume = Number(els.volume.value) / 100;
  if (audio.playing && audio.master && audio.ctx) {
    audio.master.gain.setTargetAtTime(audio.volume, audio.ctx.currentTime, 0.1);
  }
}

function onVisibilityChange() {
  // iOS moves the context to "interrupted"/"suspended" on lock or app switch
  // and does not always recover on its own.
  if (!document.hidden && audio.playing && audio.ctx && audio.ctx.state !== 'running') {
    audio.ctx.resume().catch((error) => logLine(`Audio resume failed: ${error.message}`));
  }
}

window.addEventListener('online', () => setStatus(els.bleStatus, 'Online'));
window.addEventListener('offline', () => setStatus(els.bleStatus, 'Offline'));
window.addEventListener('DOMContentLoaded', init);
