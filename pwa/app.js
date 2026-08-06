const els = {};
const state = {
  username: '',
  token: '',
  repo: null,
  repoOwner: 'CastaliaInstitute',
  bleDevice: null,
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

  els.loginForm.addEventListener('submit', onLogin);
  els.discoverRepo.addEventListener('click', onDiscoverRepo);
  els.openRepo.addEventListener('click', onOpenRepo);
  els.connectDevice.addEventListener('click', onConnectDevice);
  els.disconnectDevice.addEventListener('click', onDisconnectDevice);
  els.testOffline.addEventListener('click', onOfflineTest);
  els.registerSw.addEventListener('click', onRegisterServiceWorker);

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

window.addEventListener('online', () => setStatus(els.bleStatus, 'Online'));
window.addEventListener('offline', () => setStatus(els.bleStatus, 'Offline'));
window.addEventListener('DOMContentLoaded', init);
