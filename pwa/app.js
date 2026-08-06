const els = {};

const PERMISSION_STATE = Object.freeze({
  SIGNED_OUT: 'signed_out',
  AUTHENTICATED: 'authenticated',
  DATA_CONSENT_GRANTED: 'data_consent_granted',
  BLE_CONSENT_GRANTED: 'ble_consent_granted',
  FULL_ACCESS: 'full_access',
});

const ALLOWED_REMOTE_HOSTS = Object.freeze([
  'api.github.com',
  'api.castalia.institute',
]);

const LOCAL_DATA_KEYS = [
  'nocturne-pwa-session',
  'nocturne-offline-snapshot',
];

const state = {
  username: '',
  token: '',
  sessionToken: '',
  repo: null,
  repoOwner: 'CastaliaInstitute',
  bleDevice: null,
  repoFiles: [],
  repoCommits: [],
  consentData: false,
  consentBle: false,
  permissionState: PERMISSION_STATE.SIGNED_OUT,
};

function init() {
  els.loginForm = document.getElementById('login-form');
  els.username = document.getElementById('username');
  els.token = document.getElementById('token');
  els.consentData = document.getElementById('consent-castalia-data');
  els.consentBle = document.getElementById('consent-ble');
  els.authStatus = document.getElementById('auth-status');
  els.permissionStatus = document.getElementById('permission-status');
  els.exchangeToken = document.getElementById('exchange-token');
  els.logout = document.getElementById('logout');
  els.repoStatus = document.getElementById('repo-status');
  els.repoList = document.getElementById('repo-list');
  els.repoCommits = document.getElementById('repo-commits');
  els.bleStatus = document.getElementById('ble-status');
  els.log = document.getElementById('log');

  els.discoverRepo = document.getElementById('discover-repo');
  els.openRepo = document.getElementById('open-repo');
  els.listRepoFiles = document.getElementById('list-repo-files');
  els.listRecentChanges = document.getElementById('list-recent-changes');
  els.connectDevice = document.getElementById('connect-device');
  els.disconnectDevice = document.getElementById('disconnect-device');
  els.testOffline = document.getElementById('test-offline');
  els.registerSw = document.getElementById('register-sw');
  els.exportData = document.getElementById('export-data');
  els.deleteData = document.getElementById('delete-data');

  els.loginForm.addEventListener('submit', onLogin);
  els.discoverRepo.addEventListener('click', onDiscoverRepo);
  els.openRepo.addEventListener('click', onOpenRepo);
  els.listRepoFiles.addEventListener('click', onListRepoFiles);
  els.listRecentChanges.addEventListener('click', onListRecentChanges);
  els.connectDevice.addEventListener('click', onConnectDevice);
  els.disconnectDevice.addEventListener('click', onDisconnectDevice);
  els.exchangeToken.addEventListener('click', onExchangeToken);
  els.logout.addEventListener('click', onLogout);
  els.testOffline.addEventListener('click', onOfflineTest);
  els.registerSw.addEventListener('click', onRegisterServiceWorker);
  els.exportData.addEventListener('click', onExportData);
  els.deleteData.addEventListener('click', onDeleteData);
  els.consentData.addEventListener('change', onConsentChanged);
  els.consentBle.addEventListener('change', onConsentChanged);

  loadSession();
  onConsentChanged();
}

function gatherStoredValue(key) {
  const raw = localStorage.getItem(key);
  if (raw === null) {
    return null;
  }
  try {
    return JSON.parse(raw);
  } catch {
    return raw;
  }
}

function exportLocalDataPayload() {
  const data = {};
  for (const key of LOCAL_DATA_KEYS) {
    const value = gatherStoredValue(key);
    if (value !== null) {
      data[key] = value;
    }
  }
  return {
    exportedAt: new Date().toISOString(),
    app: 'nocturne-pwa',
    keys: data,
  };
}

function clearLocalData() {
  for (const key of LOCAL_DATA_KEYS) {
    localStorage.removeItem(key);
  }
}

function resetUiAfterSessionReset() {
  state.username = '';
  state.token = '';
  state.sessionToken = '';
  state.permissionState = PERMISSION_STATE.SIGNED_OUT;
  state.repo = null;
  state.repoFiles = [];
  state.repoCommits = [];
  state.consentData = false;
  state.consentBle = false;
  if (state.bleDevice?.gatt?.connected) {
    state.bleDevice.gatt.disconnect();
  }
  state.bleDevice = null;
  if (els.disconnectDevice) {
    els.disconnectDevice.disabled = true;
  }
  if (els.consentData) {
    els.consentData.checked = false;
  }
  if (els.consentBle) {
    els.consentBle.checked = false;
  }
  if (els.loginForm) {
    els.loginForm.reset();
  }
  els.repoList.textContent = 'Files: none yet.';
  els.repoCommits.textContent = 'Recent commits: none yet.';
  setStatus(els.bleStatus, 'No connected device');
  setStatus(els.repoStatus, 'Signed out');
  setStatus(els.authStatus, 'Session cleared');
  setStatus(els.permissionStatus, 'Permission: Signed out');
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
    state.sessionToken = session.sessionToken || '';
    state.consentData = !!session.consentData;
    state.consentBle = !!session.consentBle;
    if (state.username) {
      els.username.value = state.username;
      els.token.value = state.token;
      els.consentData.checked = state.consentData;
      els.consentBle.checked = state.consentBle;
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
    JSON.stringify({
      username: state.username,
      token: state.token,
      sessionToken: state.sessionToken,
      consentData: state.consentData,
      consentBle: state.consentBle,
    })
  );
}

function clearSession() {
  localStorage.removeItem('nocturne-pwa-session');
  resetUiAfterSessionReset();
}

function setAuthenticated() {
  setStatus(els.authStatus, `Connected as ${state.username}`);
  els.exchangeToken.disabled = false;
  applyConsentGating();
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
  state.consentData = els.consentData.checked;
  state.consentBle = els.consentBle.checked;
  saveSession();
  setAuthenticated();
  onLoginStateUpdated();
}

function onLoginStateUpdated() {
  applyConsentGating();
  setStatus(els.repoStatus, `Ready to resolve castalia-${state.username}`);
  setStatus(els.bleStatus, state.bleDevice ? `Connected: ${state.bleDevice.name}` : 'No connected device');
  setStatus(els.authStatus, `Connected as ${state.username}`);
}

function getAuthToken() {
  return state.sessionToken || state.token;
}

function getExchangeEndpoint() {
  return 'https://api.castalia.institute/nocturne/token-exchange';
}

function buildGithubEndpoint(path) {
  return `https://api.github.com/${String(path || '').replace(/^\/+/, '')}`;
}

function isAllowedNetworkTarget(urlString) {
  try {
    const parsed = new URL(urlString);
    return ALLOWED_REMOTE_HOSTS.includes(parsed.hostname);
  } catch {
    return false;
  }
}

function formatPermissionState() {
  const map = {
    [PERMISSION_STATE.SIGNED_OUT]: 'Permission: Signed out',
    [PERMISSION_STATE.AUTHENTICATED]: 'Permission: Authenticated (no permissions granted)',
    [PERMISSION_STATE.DATA_CONSENT_GRANTED]: 'Permission: Repo access granted',
    [PERMISSION_STATE.BLE_CONSENT_GRANTED]: 'Permission: BLE access granted',
    [PERMISSION_STATE.FULL_ACCESS]: 'Permission: Repo + BLE access granted',
  };
  return map[state.permissionState] || 'Permission: Unknown';
}

function computePermissionState() {
  if (!state.username || !state.token) {
    return PERMISSION_STATE.SIGNED_OUT;
  }
  if (state.consentData && state.consentBle) {
    return PERMISSION_STATE.FULL_ACCESS;
  }
  if (state.consentData) {
    return PERMISSION_STATE.DATA_CONSENT_GRANTED;
  }
  if (state.consentBle) {
    return PERMISSION_STATE.BLE_CONSENT_GRANTED;
  }
  return PERMISSION_STATE.AUTHENTICATED;
}

function enforcePermissionState() {
  state.permissionState = computePermissionState();
  setStatus(els.permissionStatus, formatPermissionState());
}

function requirePermission(context) {
  if (!state.username || !state.token) {
    setStatus(els.authStatus, 'Authentication required');
    return false;
  }
  if (context === 'repo' && !state.consentData) {
    setStatus(els.repoStatus, 'Castalia data consent required');
    return false;
  }
  if (context === 'ble' && !state.consentBle) {
    setStatus(els.bleStatus, 'BLE consent required');
    return false;
  }
  if (context === 'repo-content' && !state.repo?.full_name) {
    setStatus(els.repoStatus, 'Repo not resolved');
    return false;
  }
  return true;
}

function applyConsentGating() {
  const hasAuth = !!state.username && !!state.token;
  const repoEnabled = hasAuth && state.consentData;
  const bleEnabled = hasAuth && state.consentBle && 'bluetooth' in navigator;

  els.discoverRepo.disabled = !repoEnabled;
  els.openRepo.disabled = !repoEnabled || !state.repo;
  els.listRepoFiles.disabled = !repoEnabled || !state.repo;
  els.listRecentChanges.disabled = !repoEnabled || !state.repo;
  els.connectDevice.disabled = !bleEnabled;
  els.exchangeToken.disabled = !hasAuth;
  enforcePermissionState();
}

function onConsentChanged() {
  state.consentData = !!els.consentData.checked;
  state.consentBle = !!els.consentBle.checked;
  applyConsentGating();
  if (state.username) {
    saveSession();
  }
}

async function githubGet(url, token) {
  if (!isAllowedNetworkTarget(url)) {
    throw new Error(`Blocked network endpoint: ${url}`);
  }
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

function formatRepoListLines(items) {
  if (!Array.isArray(items) || items.length === 0) {
    return 'No workspace files';
  }
  const lines = [];
  for (const it of items) {
    lines.push(`${it.type.padEnd(7)} ${(it.name || '(unknown)').padEnd(28)} ${it.path || ''}`);
  }
  return lines.join('\n');
}

function formatCommitLines(commits) {
  if (!Array.isArray(commits) || commits.length === 0) {
    return 'No recent changes';
  }
  return commits
    .map((commit) => {
      const sha = commit.sha ? commit.sha.substring(0, 7) : '------';
      const msg = commit.commit?.message || '(no message)';
      const actor = commit.author?.login || commit.commit?.author?.name || 'unknown';
      const date = commit.commit?.author?.date || '';
      return `${sha}  ${actor.padEnd(14)}  ${date ? new Date(date).toISOString() : 'n/a'}  ${msg}`;
    })
    .join('\n');
}

async function onDiscoverRepo() {
  if (!requirePermission('repo')) {
    return;
  }
  els.discoverRepo.disabled = true;
  const activeToken = getAuthToken();
  setStatus(els.repoStatus, 'Discovering repository...');
  const repoName = repoNameForUser(state.username);
  const repoSlug = `${state.repoOwner}/${repoName}`;
  try {
    state.repo = await githubGet(buildGithubEndpoint(`/repos/${repoSlug}`), activeToken);
    els.openRepo.disabled = false;
    els.listRepoFiles.disabled = false;
    els.listRecentChanges.disabled = false;
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
  if (!requirePermission('repo-content')) return;
  if (!state.repo?.html_url) return;
  window.open(state.repo.html_url, '_blank', 'noopener,noreferrer');
}

async function onListRepoFiles() {
  if (!requirePermission('repo-content')) {
    return;
  }
  setStatus(els.repoStatus, 'Loading workspace files...');
  try {
    const apiPath = buildGithubEndpoint(`/repos/${state.repo.full_name}/contents`);
    const items = await githubGet(apiPath, getAuthToken());
    state.repoFiles = Array.isArray(items) ? items : [];
    const fileText = formatRepoListLines(state.repoFiles);
    els.repoList.textContent = fileText;
    setStatus(els.repoStatus, `Loaded ${state.repoFiles.length} repo entry(ies)`);
  } catch (error) {
    setStatus(els.repoStatus, `Failed to load files: ${error.message}`);
    logLine(`Files lookup failed: ${error.message}`);
  }
}

async function onListRecentChanges() {
  if (!requirePermission('repo-content')) {
    return;
  }
  setStatus(els.repoStatus, 'Loading recent changes...');
  try {
    const apiPath = buildGithubEndpoint(`/repos/${state.repo.full_name}/commits?per_page=5`);
    const commits = await githubGet(apiPath, getAuthToken());
    state.repoCommits = Array.isArray(commits) ? commits : [];
    els.repoCommits.textContent = formatCommitLines(state.repoCommits);
    setStatus(els.repoStatus, `Loaded ${state.repoCommits.length} commit(s)`);
  } catch (error) {
    setStatus(els.repoStatus, `Failed to load commits: ${error.message}`);
    logLine(`Commit lookup failed: ${error.message}`);
  }
}

function onLogout() {
  clearSession();
  applyConsentGating();
}

async function onExchangeToken() {
  if (!requirePermission('exchange')) {
    setStatus(els.authStatus, 'Sign in before exchange');
    return;
  }

  const exchangeEndpoint = getExchangeEndpoint();
  setStatus(els.authStatus, 'Exchanging token with Castalia...');
  if (!isAllowedNetworkTarget(exchangeEndpoint)) {
    setStatus(els.authStatus, 'Exchange endpoint blocked by policy');
    return;
  }
  try {
    const response = await fetch(exchangeEndpoint, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        Accept: 'application/json',
      },
      body: JSON.stringify({
        actor: state.username,
        provider: 'github',
        access_token: state.token,
      }),
    });
    if (!response.ok) {
      throw new Error(`token exchange endpoint returned ${response.status}`);
    }
    const data = await response.json();
    if (!data || typeof data.session_token !== 'string') {
      throw new Error('token exchange response missing session_token');
    }
    state.sessionToken = data.session_token;
    saveSession();
    setStatus(els.authStatus, 'Castalia exchange succeeded');
    logLine('Castalia token exchange succeeded');
    setAuthenticated();
  } catch (error) {
    state.sessionToken = '';
    setStatus(els.authStatus, `Exchange unavailable; using scoped PAT mode: ${error.message}`);
    logLine(`Token exchange failed, fallback to PAT: ${error.message}`);
    saveSession();
  }
}

async function onConnectDevice() {
  if (!requirePermission('ble')) {
    return;
  }
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

function onExportData() {
  const payload = exportLocalDataPayload();
  const payloadKeys = Object.keys(payload.keys);
  if (payloadKeys.length === 0) {
    setStatus(els.repoStatus, 'No local data found for export');
    return;
  }

  const filename = `nocturne-local-data-${new Date().toISOString().replace(/[:.]/g, '-')}.json`;
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = filename;
  link.rel = 'noopener noreferrer';
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
  logLine(`Exported local keys: ${payloadKeys.join(', ')}`);
  setStatus(els.repoStatus, `Exported ${payloadKeys.length} local key(s)`);
}

function onDeleteData() {
  if (!window.confirm('Delete local Nocturne data from this browser? This cannot be undone.')) {
    setStatus(els.repoStatus, 'Delete cancelled');
    return;
  }
  clearLocalData();
  clearSession();
  onConsentChanged();
  setStatus(els.authStatus, 'Local data deleted');
  setStatus(els.repoStatus, 'All local data removed');
  logLine('Local Nocturne data deleted');
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
