document.getElementById('readBtn').addEventListener('click', async () => {
  document.getElementById('dumpArea').textContent = 'Reading…';
  try {
    // POST to trigger a read
    let res = await fetch('/api/read', { method: 'POST' });
    let json = await res.json();

    // update UID
    document.getElementById('lastUid').textContent = json.uid;
    // show dump as hex
    document.getElementById('dumpArea').textContent =
      json.data.map(b => b.toString(16).padStart(2,'0')).join(' ');
  } catch (err) {
    document.getElementById('dumpArea').textContent = 'Error: ' + err;
  }
});
