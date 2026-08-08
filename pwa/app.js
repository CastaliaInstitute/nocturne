const els = {};

// Castalia identity is Supabase-backed (Google / GitHub / email magic link).
// No credentials are ever typed into this app. The Supabase client config is
// loaded at runtime from the shared Castalia config script (see index.html),
// so this app never duplicates keys and picks up rotations automatically.
function supabaseConfig() {
  const url = window.CASTALIA_SUPABASE_URL || window.BIBLIOTECH_SUPABASE_URL || '';
  const key = window.CASTALIA_SUPABASE_ANON_KEY || window.BIBLIOTECH_SUPABASE_ANON_KEY || '';
  return url && key ? { url, key } : null;
}

// Fetch the shared config as plain text and extract the two public values.
// Parsed as data — never executed. Cached for offline reuse.
const SHARED_CONFIG_URL = 'https://bibliotech.castalia.institute/config.js';
const SHARED_CONFIG_CACHE_KEY = 'nocturne-castalia-config';

function applySharedConfigText(text) {
  const url = text.match(/SUPABASE_URL\s*=\s*["']([^"']+)["']/);
  const key = text.match(/SUPABASE_ANON_KEY\s*=\s*["']([^"']+)["']/);
  if (url && key) {
    window.CASTALIA_SUPABASE_URL = url[1];
    window.CASTALIA_SUPABASE_ANON_KEY = key[1];
    return true;
  }
  return false;
}

async function loadSharedConfig() {
  if (supabaseConfig()) {
    return;
  }
  const cached = localStorage.getItem(SHARED_CONFIG_CACHE_KEY);
  if (cached && applySharedConfigText(cached)) {
    return;
  }
  try {
    const response = await fetch(SHARED_CONFIG_URL);
    if (!response.ok) {
      return;
    }
    const text = await response.text();
    if (applySharedConfigText(text)) {
      localStorage.setItem(SHARED_CONFIG_CACHE_KEY, text);
    }
  } catch {
    // Offline or config host unavailable; auth actions will report status.
  }
}

const PERMISSION_STATE = Object.freeze({
  SIGNED_OUT: 'signed_out',
  AUTHENTICATED: 'authenticated',
  DATA_CONSENT_GRANTED: 'data_consent_granted',
  BLE_CONSENT_GRANTED: 'ble_consent_granted',
  FULL_ACCESS: 'full_access',
});

function allowedRemoteHosts() {
  const hosts = ['api.github.com', 'api.castalia.institute'];
  const config = supabaseConfig();
  if (config) {
    try {
      hosts.push(new URL(config.url).hostname);
    } catch {
      // ignore malformed config URL
    }
  }
  return hosts;
}

const SESSION_KEY = 'nocturne-supabase-session';

const LOCAL_DATA_KEYS = [
  SESSION_KEY,
  'nocturne-pwa-session',
  'nocturne-offline-snapshot',
];

const state = {
  username: '',
  email: '',
  session: null, // {access_token, refresh_token, expires_at, provider_token, user}
  repo: null,
  repoOwner: 'CastaliaInstitute',
  bleDevice: null,
  repoFiles: [],
  repoCommits: [],
  consentData: false,
  consentBle: false,
  permissionState: PERMISSION_STATE.SIGNED_OUT,
};

// ---------------------------------------------------------------------------
// Shared identity cookie: lets any *.castalia.institute app pick up an
// existing Castalia login. Holds identity + Castalia session token only.
// ---------------------------------------------------------------------------

const CASTALIA_COOKIE = 'castalia_identity';

function castaliaCookieDomain() {
  const host = window.location.hostname;
  return host === 'castalia.institute' || host.endsWith('.castalia.institute')
    ? '.castalia.institute'
    : '';
}

function writeCastaliaCookie() {
  if (!state.username) {
    return;
  }
  const payload = btoa(JSON.stringify({
    u: state.username,
    s: state.session ? state.session.access_token : '',
    t: Date.now(),
  }));
  const domain = castaliaCookieDomain();
  let cookie = `${CASTALIA_COOKIE}=${payload}; Path=/; Max-Age=2592000; SameSite=Lax`;
  if (domain) {
    cookie += `; Domain=${domain}; Secure`;
  }
  document.cookie = cookie;
}

function readCastaliaCookie() {
  const match = document.cookie
    .split(';')
    .map((part) => part.trim())
    .find((part) => part.startsWith(`${CASTALIA_COOKIE}=`));
  if (!match) {
    return null;
  }
  try {
    const parsed = JSON.parse(atob(match.slice(CASTALIA_COOKIE.length + 1)));
    if (!parsed || typeof parsed.u !== 'string' || !parsed.u) {
      return null;
    }
    return { username: parsed.u, sessionToken: typeof parsed.s === 'string' ? parsed.s : '' };
  } catch {
    return null;
  }
}

function clearCastaliaCookie() {
  const domain = castaliaCookieDomain();
  let cookie = `${CASTALIA_COOKIE}=; Path=/; Max-Age=0; SameSite=Lax`;
  if (domain) {
    cookie += `; Domain=${domain}; Secure`;
  }
  document.cookie = cookie;
}

function adoptSharedIdentity() {
  if (state.username) {
    return; // local session wins
  }
  const shared = readCastaliaCookie();
  if (!shared) {
    return;
  }
  state.username = shared.username;
  if (shared.sessionToken) {
    state.session = { access_token: shared.sessionToken, shared: true };
  }
  setStatus(els.authStatus, `Connected as ${state.username} (shared Castalia session)`);
  logLine(`Adopted shared Castalia identity: ${state.username}`);
}

// ---------------------------------------------------------------------------
// Supabase auth (implicit flow, no SDK): Google, GitHub, email magic link.
// ---------------------------------------------------------------------------

function requireSupabase() {
  const config = supabaseConfig();
  if (!config) {
    setStatus(els.authStatus, 'Castalia auth config unavailable (offline?)');
    return null;
  }
  return config;
}

function supabaseHeaders(config, withAuth) {
  const headers = { apikey: config.key, 'Content-Type': 'application/json' };
  if (withAuth && state.session) {
    headers.Authorization = `Bearer ${state.session.access_token}`;
  }
  return headers;
}

function usernameFromUser(user) {
  if (!user) return '';
  const meta = user.user_metadata || {};
  return (
    meta.user_name ||
    meta.preferred_username ||
    (user.email ? user.email.split('@')[0] : '') ||
    ''
  );
}

function beginOAuth(provider) {
  const config = requireSupabase();
  if (!config) return;
  const redirect = encodeURIComponent(`${window.location.origin}/`);
  window.location.href =
    `${config.url}/auth/v1/authorize?provider=${provider}&redirect_to=${redirect}`;
}

async function sendMagicLink(email) {
  const config = requireSupabase();
  if (!config) {
    throw new Error('auth config unavailable');
  }
  const redirect = encodeURIComponent(`${window.location.origin}/`);
  const response = await fetch(`${config.url}/auth/v1/otp?redirect_to=${redirect}`, {
    method: 'POST',
    headers: supabaseHeaders(config, false),
    body: JSON.stringify({ email, create_user: true }),
  });
  if (!response.ok) {
    const body = await response.text();
    throw new Error(`magic link request failed (${response.status}): ${body.slice(0, 160)}`);
  }
}

function parseAuthRedirectHash() {
  const hash = window.location.hash.replace(/^#/, '');
  if (!hash || !hash.includes('access_token=')) {
    return null;
  }
  const params = new URLSearchParams(hash);
  const accessToken = params.get('access_token');
  if (!accessToken) {
    return null;
  }
  const expiresIn = parseInt(params.get('expires_in') || '3600', 10);
  return {
    access_token: accessToken,
    refresh_token: params.get('refresh_token') || '',
    provider_token: params.get('provider_token') || '',
    expires_at: Math.floor(Date.now() / 1000) + expiresIn,
  };
}

async function fetchSupabaseUser() {
  const config = requireSupabase();
  if (!config) {
    throw new Error('auth config unavailable');
  }
  const response = await fetch(`${config.url}/auth/v1/user`, {
    headers: supabaseHeaders(config, true),
  });
  if (!response.ok) {
    throw new Error(`user lookup failed (${response.status})`);
  }
  return response.json();
}

async function refreshSupabaseSession() {
  const config = supabaseConfig();
  if (!config || !state.session || !state.session.refresh_token) {
    return false;
  }
  try {
    const response = await fetch(`${config.url}/auth/v1/token?grant_type=refresh_token`, {
      method: 'POST',
      headers: supabaseHeaders(config, false),
      body: JSON.stringify({ refresh_token: state.session.refresh_token }),
    });
    if (!response.ok) {
      return false;
    }
    const data = await response.json();
    state.session = {
      access_token: data.access_token,
      refresh_token: data.refresh_token || state.session.refresh_token,
      provider_token: state.session.provider_token || '',
      expires_at: Math.floor(Date.now() / 1000) + (data.expires_in || 3600),
      user: data.user || state.session.user,
    };
    saveSession();
    return true;
  } catch {
    return false;
  }
}

async function completeSignIn(session) {
  state.session = session;
  try {
    const user = await fetchSupabaseUser();
    state.session.user = user;
    state.username = usernameFromUser(user);
    state.email = user.email || '';
  } catch (error) {
    logLine(`User lookup failed: ${error.message}`);
  }
  saveSession();
  writeCastaliaCookie();
  setAuthenticated();
  applyConsentGating();
  updateConnectButton();
}

async function handleAuthRedirect() {
  const session = parseAuthRedirectHash();
  if (!session) {
    return false;
  }
  window.history.replaceState(null, '', window.location.pathname + window.location.search);
  await completeSignIn(session);
  logLine('Signed in via Castalia (Supabase)');
  openDrawer();
  return true;
}

// ---------------------------------------------------------------------------
// Local session persistence
// ---------------------------------------------------------------------------

function loadSession() {
  const raw = localStorage.getItem(SESSION_KEY);
  if (!raw) {
    return;
  }
  try {
    const stored = JSON.parse(raw);
    if (!stored || !stored.access_token) {
      return;
    }
    state.session = stored;
    state.username = usernameFromUser(stored.user);
    state.email = stored.user ? stored.user.email || '' : '';
    state.consentData = !!stored.consentData;
    state.consentBle = !!stored.consentBle;
    if (els.consentData) els.consentData.checked = state.consentData;
    if (els.consentBle) els.consentBle.checked = state.consentBle;

    const now = Math.floor(Date.now() / 1000);
    if (stored.expires_at && stored.expires_at < now + 60) {
      refreshSupabaseSession().then((ok) => {
        if (ok) {
          setAuthenticated();
          applyConsentGating();
          updateConnectButton();
        } else {
          logLine('Stored Castalia session expired');
        }
      });
    }
    if (state.username) {
      setAuthenticated();
    }
  } catch (error) {
    logLine(`Failed to parse cached session: ${error.message}`);
  }
}

function saveSession() {
  if (!state.session) {
    return;
  }
  localStorage.setItem(SESSION_KEY, JSON.stringify({
    ...state.session,
    consentData: state.consentData,
    consentBle: state.consentBle,
  }));
}

function clearSession() {
  localStorage.removeItem(SESSION_KEY);
  localStorage.removeItem('nocturne-pwa-session');
  clearCastaliaCookie();
  resetUiAfterSessionReset();
  updateConnectButton();
}

// ---------------------------------------------------------------------------
// UI wiring
// ---------------------------------------------------------------------------

function init() {
  els.magicForm = document.getElementById('magic-form');
  els.email = document.getElementById('email');
  els.googleSignin = document.getElementById('google-signin');
  els.githubSignin = document.getElementById('github-signin');
  els.consentData = document.getElementById('consent-castalia-data');
  els.consentBle = document.getElementById('consent-ble');
  els.authStatus = document.getElementById('auth-status');
  els.permissionStatus = document.getElementById('permission-status');
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

  els.googleSignin.addEventListener('click', () => beginOAuth('google'));
  els.githubSignin.addEventListener('click', () => beginOAuth('github'));
  els.magicForm.addEventListener('submit', onSendMagicLink);
  els.discoverRepo.addEventListener('click', onDiscoverRepo);
  els.openRepo.addEventListener('click', onOpenRepo);
  els.listRepoFiles.addEventListener('click', onListRepoFiles);
  els.listRecentChanges.addEventListener('click', onListRecentChanges);
  els.connectDevice.addEventListener('click', onConnectDevice);
  els.disconnectDevice.addEventListener('click', onDisconnectDevice);
  els.logout.addEventListener('click', onLogout);
  els.testOffline.addEventListener('click', onOfflineTest);
  els.registerSw.addEventListener('click', onRegisterServiceWorker);
  els.exportData.addEventListener('click', onExportData);
  els.deleteData.addEventListener('click', onDeleteData);
  els.consentData.addEventListener('change', onConsentChanged);
  els.consentBle.addEventListener('change', onConsentChanged);

  initDrawer();
  loadSession();
  loadSharedConfig()
    .then(() => handleAuthRedirect())
    .then((signedIn) => {
      if (!signedIn) {
        adoptSharedIdentity();
      }
      onConsentChanged();
      updateConnectButton();
    });
  autoRegisterServiceWorker();
}

function initDrawer() {
  els.drawer = document.getElementById('connect-drawer');
  els.drawerBackdrop = document.getElementById('drawer-backdrop');
  els.openConnect = document.getElementById('open-connect');
  els.closeDrawer = document.getElementById('close-drawer');
  if (!els.drawer || !els.openConnect) {
    return;
  }
  els.openConnect.addEventListener('click', openDrawer);
  els.closeDrawer.addEventListener('click', closeDrawer);
  els.drawerBackdrop.addEventListener('click', closeDrawer);
  window.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      closeDrawer();
    }
  });
}

function openDrawer() {
  if (!els.drawer) return;
  els.drawerBackdrop.hidden = false;
  // Force a reflow so the un-hidden backdrop transitions cleanly, then add
  // classes synchronously (rAF would stall in hidden/background tabs).
  void els.drawerBackdrop.offsetHeight;
  els.drawerBackdrop.classList.add('visible');
  els.drawer.classList.add('open');
  els.drawer.setAttribute('aria-hidden', 'false');
  if (!state.username && els.email) {
    els.email.focus();
  }
}

function closeDrawer() {
  if (!els.drawer) return;
  els.drawer.classList.remove('open');
  els.drawerBackdrop.classList.remove('visible');
  els.drawer.setAttribute('aria-hidden', 'true');
  setTimeout(() => {
    if (els.drawerBackdrop && !els.drawerBackdrop.classList.contains('visible')) {
      els.drawerBackdrop.hidden = true;
    }
  }, 400);
}

function updateConnectButton() {
  if (!els.openConnect) {
    return;
  }
  if (state.username) {
    els.openConnect.textContent = state.username;
    els.openConnect.classList.add('connected');
  } else {
    els.openConnect.textContent = 'Connect to Castalia';
    els.openConnect.classList.remove('connected');
  }
}

async function autoRegisterServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    return;
  }
  try {
    await navigator.serviceWorker.register('./service-worker.js', { scope: './' });
  } catch {
    // Manual registration remains available from the drawer.
  }
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
  state.email = '';
  state.session = null;
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
  if (els.magicForm) {
    els.magicForm.reset();
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

function setAuthenticated() {
  const label = state.email && state.email !== state.username
    ? `${state.username} (${state.email})`
    : state.username;
  setStatus(els.authStatus, `Connected as ${label}`);
  applyConsentGating();
}

async function onSendMagicLink(event) {
  event.preventDefault();
  const email = (els.email.value || '').trim();
  if (!email) {
    setStatus(els.authStatus, 'Enter an email for the magic link.');
    return;
  }
  setStatus(els.authStatus, 'Sending magic link...');
  try {
    await sendMagicLink(email);
    setStatus(els.authStatus, `Magic link sent to ${email}. Open it on this device.`);
    logLine(`Magic link requested for ${email}`);
  } catch (error) {
    setStatus(els.authStatus, `Magic link failed: ${error.message}`);
    logLine(`Magic link failed: ${error.message}`);
  }
}

function getAuthToken() {
  return state.session ? state.session.access_token : '';
}

function getProviderToken() {
  return state.session ? state.session.provider_token || '' : '';
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
  if (!state.username || !getAuthToken()) {
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
  if (!state.username || !getAuthToken()) {
    setStatus(els.authStatus, 'Sign in with Castalia first');
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
  const hasAuth = !!state.username && !!getAuthToken();
  const repoEnabled = hasAuth && state.consentData;
  const bleEnabled = hasAuth && state.consentBle && 'bluetooth' in navigator;

  els.discoverRepo.disabled = !repoEnabled;
  els.openRepo.disabled = !repoEnabled || !state.repo;
  els.listRepoFiles.disabled = !repoEnabled || !state.repo;
  els.listRecentChanges.disabled = !repoEnabled || !state.repo;
  els.connectDevice.disabled = !bleEnabled;
  enforcePermissionState();
}

function onConsentChanged() {
  state.consentData = !!els.consentData.checked;
  state.consentBle = !!els.consentBle.checked;
  applyConsentGating();
  if (state.session) {
    saveSession();
  }
}

// ---------------------------------------------------------------------------
// Repo access. Preferred path: the Castalia workspace API with the Castalia
// session JWT. Fallback: when signed in via GitHub OAuth, Supabase provides a
// provider token that reads the per-user repo directly — still nothing typed.
// ---------------------------------------------------------------------------

function isAllowedNetworkTarget(urlString) {
  try {
    const parsed = new URL(urlString);
    return allowedRemoteHosts().includes(parsed.hostname);
  } catch {
    return false;
  }
}

async function castaliaGet(path) {
  const url = `https://api.castalia.institute/nocturne/${String(path).replace(/^\/+/, '')}`;
  if (!isAllowedNetworkTarget(url)) {
    throw new Error(`Blocked network endpoint: ${url}`);
  }
  const response = await fetch(url, {
    headers: {
      Accept: 'application/json',
      Authorization: `Bearer ${getAuthToken()}`,
    },
  });
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return response.json();
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
    lines.push(`${(it.type || 'file').padEnd(7)} ${(it.name || '(unknown)').padEnd(28)} ${it.path || ''}`);
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
      const msg = commit.commit?.message || commit.message || '(no message)';
      const actor = commit.author?.login || commit.commit?.author?.name || commit.author || 'unknown';
      const date = commit.commit?.author?.date || commit.date || '';
      return `${sha}  ${String(actor).padEnd(14)}  ${date ? new Date(date).toISOString() : 'n/a'}  ${msg}`;
    })
    .join('\n');
}

async function onDiscoverRepo() {
  if (!requirePermission('repo')) {
    return;
  }
  els.discoverRepo.disabled = true;
  setStatus(els.repoStatus, 'Discovering repository...');
  const repoName = repoNameForUser(state.username);
  const repoSlug = `${state.repoOwner}/${repoName}`;
  try {
    try {
      state.repo = await castaliaGet('repo');
    } catch {
      const providerToken = getProviderToken();
      if (!providerToken) {
        throw new Error(
          'Castalia workspace API unreachable. Sign in with GitHub to browse the repo directly.'
        );
      }
      state.repo = await githubGet(`https://api.github.com/repos/${repoSlug}`, providerToken);
    }
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
    let items;
    try {
      items = await castaliaGet('repo/files');
    } catch {
      const providerToken = getProviderToken();
      if (!providerToken) {
        throw new Error('workspace API unreachable; GitHub sign-in enables direct browsing');
      }
      items = await githubGet(`https://api.github.com/repos/${state.repo.full_name}/contents`, providerToken);
    }
    state.repoFiles = Array.isArray(items) ? items : [];
    els.repoList.textContent = formatRepoListLines(state.repoFiles);
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
    let commits;
    try {
      commits = await castaliaGet('repo/commits');
    } catch {
      const providerToken = getProviderToken();
      if (!providerToken) {
        throw new Error('workspace API unreachable; GitHub sign-in enables direct browsing');
      }
      commits = await githubGet(
        `https://api.github.com/repos/${state.repo.full_name}/commits?per_page=5`,
        providerToken
      );
    }
    state.repoCommits = Array.isArray(commits) ? commits : [];
    els.repoCommits.textContent = formatCommitLines(state.repoCommits);
    setStatus(els.repoStatus, `Loaded ${state.repoCommits.length} commit(s)`);
  } catch (error) {
    setStatus(els.repoStatus, `Failed to load commits: ${error.message}`);
    logLine(`Commit lookup failed: ${error.message}`);
  }
}

function onLogout() {
  const config = supabaseConfig();
  if (config && getAuthToken() && !state.session?.shared) {
    fetch(`${config.url}/auth/v1/logout`, {
      method: 'POST',
      headers: supabaseHeaders(config, true),
    }).catch(() => {});
  }
  clearSession();
  applyConsentGating();
}

// ---------------------------------------------------------------------------
// BLE, offline tools, service worker
// ---------------------------------------------------------------------------

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
