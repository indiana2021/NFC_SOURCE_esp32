async function refreshStatus() {
  try {
    const res = await fetch('/api/status');
    const json = await res.json();
    document.getElementById('currentMode').textContent = json.mode;
    document.getElementById('lastUid').textContent = json.uid || '—';
    document.getElementById('dumpArea').textContent =
      (json.dump || []).map(b => b.toString(16).padStart(2, '0')).join(' ');
  } catch (err) {
    console.error(err);
  }
}

document.getElementById('refreshBtn').addEventListener('click', refreshStatus);

document.getElementById('readBtn').addEventListener('click', async () => {
  document.getElementById('dumpArea').textContent = 'Reading…';
  try {
    let res = await fetch('/api/read', { method: 'POST' });
    let json = await res.json();
    document.getElementById('lastUid').textContent = json.uid;
    document.getElementById('dumpArea').textContent =
      json.data.map(b => b.toString(16).padStart(2, '0')).join(' ');
  } catch (err) {
    document.getElementById('dumpArea').textContent = 'Error: ' + err;
  }
});

document.getElementById('setModeBtn').addEventListener('click', async () => {
  const mode = document.getElementById('modeSelect').value;
  await fetch('/api/mode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'mode=' + encodeURIComponent(mode)
  });
  refreshStatus();
});

window.addEventListener('load', refreshStatus);
