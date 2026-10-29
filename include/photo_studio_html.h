#pragma once
// ============================================================================
// DeskBuddy — Photo Studio Web App (HTML5 + Real-Time Live OLED Sync)
// ============================================================================
#include <Arduino.h>

static const char PHOTO_STUDIO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>DeskBuddy Photo Studio</title>
<style>
  :root {
    --bg: #0d0f12;
    --card: #161b22;
    --border: #30363d;
    --accent: #00E676;
    --accent-hover: #00C853;
    --text: #c9d1d9;
    --text-bright: #ffffff;
    --oled-bg: #000000;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: var(--bg);
    color: var(--text);
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 14px;
    min-height: 100vh;
  }
  header {
    text-align: center;
    margin-bottom: 14px;
  }
  h1 {
    font-size: 21px;
    color: var(--accent);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
  }
  .badge {
    font-size: 11px;
    background: rgba(0,230,118,0.15);
    color: var(--accent);
    padding: 3px 8px;
    border-radius: 12px;
    border: 1px solid var(--accent);
    margin-top: 4px;
    display: inline-block;
  }
  .container {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 14px;
    padding: 16px;
    width: 100%;
    max-width: 440px;
    box-shadow: 0 8px 24px rgba(0,0,0,0.5);
  }
  /* OLED Preview Screen */
  .preview-wrapper {
    display: flex;
    flex-direction: column;
    align-items: center;
    margin-bottom: 14px;
  }
  .oled-bezel {
    background: #050505;
    border: 3px solid #2a2a2a;
    border-radius: 8px;
    padding: 6px;
    box-shadow: 0 4px 16px rgba(0,0,0,0.8), inset 0 0 8px rgba(0,0,0,0.9);
    position: relative;
    touch-action: none;
    cursor: grab;
  }
  .oled-bezel:active { cursor: grabbing; }
  canvas {
    display: block;
    width: 320px;
    height: 160px;
    image-rendering: pixelated;
    background: #000;
  }
  .drag-hint {
    font-size: 11px;
    color: #8b949e;
    margin-top: 6px;
    text-align: center;
  }
  /* Upload button */
  .btn-upload {
    background: #21262d;
    border: 1px dashed var(--accent);
    color: var(--accent);
    padding: 12px;
    width: 100%;
    border-radius: 8px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
    text-align: center;
    margin-bottom: 14px;
    transition: 0.2s;
    display: block;
  }
  .btn-upload:hover { background: rgba(0,230,118,0.1); }
  input[type="file"] { display: none; }
  
  /* Control sliders */
  .control-group {
    margin-bottom: 12px;
  }
  .control-header {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    font-weight: 600;
    margin-bottom: 4px;
    color: var(--text-bright);
  }
  .control-header span.val { color: var(--accent); }
  input[type="range"] {
    width: 100%;
    accent-color: var(--accent);
  }
  
  /* Zoom preset row */
  .zoom-btn-row {
    display: flex;
    gap: 6px;
    margin-top: 6px;
  }
  .mini-btn {
    flex: 1;
    padding: 6px 4px;
    background: #21262d;
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text-bright);
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    text-align: center;
    transition: 0.15s;
    white-space: nowrap;
  }
  .mini-btn:hover { background: rgba(0,230,118,0.15); border-color: var(--accent); }

  .row-controls {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
  }
  select, button.action-btn {
    width: 100%;
    padding: 8px;
    background: #21262d;
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text-bright);
    font-size: 12px;
    cursor: pointer;
  }
  select:focus { outline: none; border-color: var(--accent); }

  /* Live Sync Card / Checkbox */
  .live-sync-card {
    background: #1c2128;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 10px 12px;
    margin-top: 14px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  .live-sync-label {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 13px;
    font-weight: 600;
    color: var(--text-bright);
    cursor: pointer;
    user-select: none;
  }
  .live-sync-label input[type="checkbox"] {
    width: 18px;
    height: 18px;
    accent-color: var(--accent);
    cursor: pointer;
  }
  .live-status-pill {
    display: flex;
    align-items: center;
    gap: 5px;
    font-size: 11px;
    font-weight: 600;
    color: var(--accent);
    background: rgba(0,230,118,0.12);
    padding: 3px 8px;
    border-radius: 12px;
    border: 1px solid rgba(0,230,118,0.3);
  }
  .live-status-pill.paused {
    color: #8b949e;
    background: rgba(139,148,158,0.1);
    border-color: #30363d;
  }
  .pulse-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--accent);
    box-shadow: 0 0 6px var(--accent);
    animation: livePulse 1.5s infinite;
  }
  .live-status-pill.paused .pulse-dot {
    background: #6e7681;
    box-shadow: none;
    animation: none;
  }
  @keyframes livePulse {
    0% { transform: scale(0.9); opacity: 0.7; }
    50% { transform: scale(1.3); opacity: 1; }
    100% { transform: scale(0.9); opacity: 0.7; }
  }

  .btn-send {
    background: var(--accent);
    color: #000;
    border: none;
    padding: 14px;
    width: 100%;
    border-radius: 8px;
    font-size: 15px;
    font-weight: 700;
    cursor: pointer;
    margin-top: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    transition: 0.2s;
  }
  .btn-send:hover { background: var(--accent-hover); }
  .btn-send:disabled { background: #333; color: #666; cursor: not-allowed; }
  
  /* Status Toast */
  #toast {
    position: fixed;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: #1f6feb;
    color: #fff;
    padding: 10px 20px;
    border-radius: 30px;
    font-size: 13px;
    font-weight: 600;
    box-shadow: 0 4px 12px rgba(0,0,0,0.5);
    transition: 0.3s;
    z-index: 999;
  }
  #toast.show { transform: translateX(-50%) translateY(0); }
</style>
</head>
<body>

<header>
  <h1>📸 DeskBuddy Photo Studio</h1>
  <div class="badge" id="statusBadge">Live OLED Companion</div>
</header>

<div class="container">
  <div class="preview-wrapper">
    <div class="oled-bezel" id="bezel">
      <canvas id="previewCanvas" width="128" height="64"></canvas>
    </div>
    <div class="drag-hint">👆 Drag to Pan &bull; Wheel / Pinch to Zoom</div>
  </div>

  <label class="btn-upload">
    📁 Pick Photo or Take Picture
    <input type="file" id="fileInput" accept="image/*">
  </label>

  <!-- Zoom Controls -->
  <div class="control-group">
    <div class="control-header">
      <span>Zoom Level</span>
      <span class="val" id="zoomVal">1.0x (Fit)</span>
    </div>
    <input type="range" id="zoomSlider" min="0.1" max="4.0" step="0.05" value="1.0">
    <div class="zoom-btn-row">
      <button type="button" class="mini-btn" id="btnZoomOut">➖ Out</button>
      <button type="button" class="mini-btn" id="btnFit">🔍 Fit All</button>
      <button type="button" class="mini-btn" id="btnFill">⛶ Fill</button>
      <button type="button" class="mini-btn" id="btnZoomIn">➕ In</button>
      <button type="button" class="mini-btn" id="btnReset">↺ Center</button>
    </div>
  </div>

  <div class="control-group">
    <div class="control-header">
      <span>Brightness</span>
      <span class="val" id="brightVal">0</span>
    </div>
    <input type="range" id="brightSlider" min="-100" max="100" step="2" value="0">
  </div>

  <div class="control-group">
    <div class="control-header">
      <span>Contrast</span>
      <span class="val" id="contrastVal">1.2x</span>
    </div>
    <input type="range" id="contrastSlider" min="0.5" max="3.0" step="0.1" value="1.2">
  </div>

  <div class="row-controls">
    <div class="control-group">
      <div class="control-header"><span>Dither Algorithm</span></div>
      <select id="ditherMode">
        <option value="floyd" selected>Floyd-Steinberg (Photo/Selfie)</option>
        <option value="atkinson">Atkinson (Vintage Retro)</option>
        <option value="threshold">Threshold (Clean B&amp;W / Logos)</option>
      </select>
    </div>
    <div class="control-group">
      <div class="control-header"><span>Actions</span></div>
      <div style="display:flex; gap:6px;">
        <button class="action-btn" id="rotateBtn">↺ Rotate</button>
        <button class="action-btn" id="invertBtn">◐ Invert</button>
      </div>
    </div>
  </div>

  <!-- Real-Time Live Sync Checkbox -->
  <div class="live-sync-card">
    <label class="live-sync-label">
      <input type="checkbox" id="liveSyncCheck" checked>
      <span>⚡ Real-Time Live OLED Sync</span>
    </label>
    <div class="live-status-pill" id="liveStatusPill">
      <span class="pulse-dot"></span> Live
    </div>
  </div>

  <button class="btn-send" id="sendBtn" disabled>
    💾 Save to DeskBuddy (Permanent)
  </button>
</div>

<!-- Laptop / Server Monitor Settings Card -->
<div class="card" style="margin-top: 14px;">
  <div class="control-header" style="margin-bottom: 8px;">
    <span style="font-size:13px; font-weight:700;">💻 Laptop Hardware Monitor</span>
    <span class="val" id="serverStatusBadge">Port 34001</span>
  </div>
  <p style="font-size:11px; color:#888; margin-bottom:10px;">
    Double-click <code>tools/run_laptop_monitor.bat</code> on your laptop to stream live CPU, RAM &amp; Network speeds to DeskBuddy.
  </p>
  <div class="control-group" style="margin-bottom:8px;">
    <div class="control-header"><span>Laptop Monitor URL</span></div>
    <input type="text" id="serverUrlInput" placeholder="http://10.91.8.136:34001/api/v1.0/get-history?n=1" style="width:100%; padding:8px; background:#111; border:1px solid #333; color:#eee; border-radius:6px; font-family:monospace; font-size:11px;">
  </div>
  <div class="control-group" style="margin-bottom:10px;">
    <div class="control-header"><span>Display Name</span></div>
    <input type="text" id="serverNameInput" placeholder="MY LAPTOP" style="width:100%; padding:8px; background:#111; border:1px solid #333; color:#eee; border-radius:6px; font-family:sans-serif; font-size:11px;">
  </div>
  <button type="button" class="action-btn" id="saveServerBtn" style="width:100%; padding:9px; background:#00E676; color:#000; font-weight:700; border-radius:8px;">
    💾 Save Laptop Settings
  </button>
</div>

<div id="toast"></div>

<!-- Offscreen scratch canvas for image math -->
<canvas id="scratchCanvas" width="128" height="64" style="display:none;"></canvas>

<script>
  const canvas = document.getElementById('previewCanvas');
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  const scratch = document.getElementById('scratchCanvas');
  const sCtx = scratch.getContext('2d', { willReadFrequently: true });
  
  const fileInput = document.getElementById('fileInput');
  const zoomSlider = document.getElementById('zoomSlider');
  const brightSlider = document.getElementById('brightSlider');
  const contrastSlider = document.getElementById('contrastSlider');
  const ditherMode = document.getElementById('ditherMode');
  const rotateBtn = document.getElementById('rotateBtn');
  const invertBtn = document.getElementById('invertBtn');
  const sendBtn = document.getElementById('sendBtn');
  const toast = document.getElementById('toast');
  const liveSyncCheck = document.getElementById('liveSyncCheck');
  const liveStatusPill = document.getElementById('liveStatusPill');
  
  const btnZoomOut = document.getElementById('btnZoomOut');
  const btnZoomIn = document.getElementById('btnZoomIn');
  const btnFit = document.getElementById('btnFit');
  const btnFill = document.getElementById('btnFill');
  const btnReset = document.getElementById('btnReset');
  
  let loadedImg = null;
  let panX = 0, panY = 0;
  let zoom = 1.0;
  let baseScale = 1.0;
  let fitScale = 1.0;
  let fillScale = 1.0;
  let rotationDeg = 0;
  let isInverted = false;
  
  // Dragging state
  let isDragging = false;
  let startX = 0, startY = 0;
  let initPanX = 0, initPanY = 0;

  // Real-time Live Sync throttling
  let isSendingLive = false;
  let pendingLiveFrame = false;
  let lastLiveSend = 0;
  const LIVE_INTERVAL_MS = 60; // Up to 15 updates per second

  function showToast(msg, bg='#1f6feb') {
    toast.innerText = msg;
    toast.style.background = bg;
    toast.classList.add('show');
    setTimeout(() => toast.classList.remove('show'), 2500);
  }

  function drawDefaultGrid() {
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, 128, 64);
    ctx.strokeStyle = '#222';
    ctx.strokeRect(1, 1, 126, 62);
    ctx.fillStyle = '#00E676';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Select a Photo Above', 64, 32);
    ctx.fillStyle = '#666';
    ctx.font = '8px sans-serif';
    ctx.fillText('128 x 64 OLED Live Canvas', 64, 44);
  }
  drawDefaultGrid();

  function fetchCityStatus() {
    fetch('/api/location')
      .then(r => r.json())
      .then(d => {
        if (d.status === 'ok' && d.city) {
          const badge = document.getElementById('statusBadge');
          if (badge) badge.innerText = '📍 ' + d.city.toUpperCase() + ' (Auto-Located)';
        }
      }).catch(()=>{});
  }
  fetchCityStatus();

  function fetchServerConfig() {
    fetch('/api/server')
      .then(r => r.json())
      .then(d => {
        if (d.status === 'ok') {
          if (d.url) document.getElementById('serverUrlInput').value = d.url;
          if (d.name) document.getElementById('serverNameInput').value = d.name;
        }
      }).catch(()=>{});
  }
  fetchServerConfig();

  const saveServerBtn = document.getElementById('saveServerBtn');
  if (saveServerBtn) {
    saveServerBtn.addEventListener('click', () => {
      const url = document.getElementById('serverUrlInput').value.trim();
      const name = document.getElementById('serverNameInput').value.trim();
      const formData = new FormData();
      formData.append('url', url);
      formData.append('name', name);
      fetch('/api/server', { method: 'POST', body: formData })
        .then(r => r.json())
        .then(d => {
          if (d.status === 'ok') showToast('Laptop Monitor URL Saved!');
          else showToast('Save failed');
        }).catch(() => showToast('Error saving settings'));
    });
  }

  function calcBaseScale() {
    if (!loadedImg) return;
    const isRot = (rotationDeg % 180 !== 0);
    const imgW = isRot ? loadedImg.height : loadedImg.width;
    const imgH = isRot ? loadedImg.width : loadedImg.height;
    
    fitScale = Math.min(128 / imgW, 64 / imgH);
    fillScale = Math.max(128 / imgW, 64 / imgH);
    baseScale = fitScale;
  }

  liveSyncCheck.addEventListener('change', () => {
    if (liveSyncCheck.checked) {
      liveStatusPill.classList.remove('paused');
      liveStatusPill.innerHTML = '<span class="pulse-dot"></span> Live';
      triggerLiveSync();
      showToast('Live OLED Sync Enabled');
    } else {
      liveStatusPill.classList.add('paused');
      liveStatusPill.innerHTML = '<span class="pulse-dot"></span> Paused';
      showToast('Live Sync Paused');
    }
  });

  fileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (ev) => {
      loadedImg = new Image();
      loadedImg.onload = () => {
        rotationDeg = 0;
        isInverted = false;
        calcBaseScale();
        zoom = 1.0;
        zoomSlider.value = "1.0";
        document.getElementById('zoomVal').innerText = '1.0x (Fit All)';
        panX = 0; panY = 0;
        sendBtn.disabled = false;
        renderImage();
        showToast('Photo Loaded! Real-time syncing...');
      };
      loadedImg.src = ev.target.result;
    };
    reader.readAsDataURL(file);
  });

  // Slider events
  zoomSlider.addEventListener('input', (e) => {
    zoom = parseFloat(e.target.value);
    document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x';
    renderImage();
  });
  
  brightSlider.addEventListener('input', (e) => {
    document.getElementById('brightVal').innerText = e.target.value;
    renderImage();
  });
  contrastSlider.addEventListener('input', (e) => {
    document.getElementById('contrastVal').innerText = parseFloat(e.target.value).toFixed(1) + 'x';
    renderImage();
  });
  ditherMode.addEventListener('change', renderImage);
  
  // Quick Zoom Buttons
  btnZoomOut.addEventListener('click', () => {
    zoom = Math.max(0.1, zoom - 0.2);
    zoomSlider.value = zoom.toFixed(2);
    document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x';
    renderImage();
  });
  btnZoomIn.addEventListener('click', () => {
    zoom = Math.min(4.0, zoom + 0.2);
    zoomSlider.value = zoom.toFixed(2);
    document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x';
    renderImage();
  });
  btnFit.addEventListener('click', () => {
    calcBaseScale();
    zoom = 1.0;
    panX = 0; panY = 0;
    zoomSlider.value = "1.0";
    document.getElementById('zoomVal').innerText = '1.0x (Fit All)';
    renderImage();
  });
  btnFill.addEventListener('click', () => {
    calcBaseScale();
    zoom = fitScale > 0 ? (fillScale / fitScale) : 1.0;
    panX = 0; panY = 0;
    zoomSlider.value = zoom.toFixed(2);
    document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x (Fill)';
    renderImage();
  });
  btnReset.addEventListener('click', () => {
    panX = 0; panY = 0;
    isInverted = false;
    renderImage();
  });

  rotateBtn.addEventListener('click', () => {
    rotationDeg = (rotationDeg + 90) % 360;
    calcBaseScale();
    renderImage();
  });
  invertBtn.addEventListener('click', () => {
    isInverted = !isInverted;
    renderImage();
  });

  // Pan / Drag interaction
  const bezel = document.getElementById('bezel');
  bezel.addEventListener('pointerdown', (e) => {
    if (!loadedImg) return;
    isDragging = true;
    startX = e.clientX;
    startY = e.clientY;
    initPanX = panX;
    initPanY = panY;
    bezel.setPointerCapture(e.pointerId);
  });

  bezel.addEventListener('pointermove', (e) => {
    if (!isDragging || !loadedImg) return;
    const dx = (e.clientX - startX) / 2.5;
    const dy = (e.clientY - startY) / 2.5;
    panX = initPanX + dx;
    panY = initPanY + dy;
    renderImage();
  });

  bezel.addEventListener('pointerup', () => { isDragging = false; });
  bezel.addEventListener('pointercancel', () => { isDragging = false; });

  // Mouse wheel zoom
  bezel.addEventListener('wheel', (e) => {
    if (!loadedImg) return;
    e.preventDefault();
    if (e.deltaY < 0) {
      zoom = Math.min(4.0, zoom + 0.1);
    } else {
      zoom = Math.max(0.1, zoom - 0.1);
    }
    zoomSlider.value = zoom.toFixed(2);
    document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x';
    renderImage();
  }, { passive: false });

  // Touch pinch-to-zoom
  let touchStartDist = 0;
  let touchStartZoom = 1.0;
  bezel.addEventListener('touchstart', (e) => {
    if (!loadedImg) return;
    if (e.touches.length === 2) {
      isDragging = false;
      touchStartDist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
      touchStartZoom = zoom;
    }
  });
  bezel.addEventListener('touchmove', (e) => {
    if (!loadedImg) return;
    if (e.touches.length === 2 && touchStartDist > 0) {
      e.preventDefault();
      const dist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
      const factor = dist / touchStartDist;
      zoom = Math.max(0.1, Math.min(4.0, touchStartZoom * factor));
      zoomSlider.value = zoom.toFixed(2);
      document.getElementById('zoomVal').innerText = zoom.toFixed(1) + 'x';
      renderImage();
    }
  }, { passive: false });

  function renderImage() {
    if (!loadedImg) return;
    const W = 128, H = 64;
    
    // Draw raw transformed image onto scratch canvas
    sCtx.save();
    sCtx.fillStyle = '#000000';
    sCtx.fillRect(0, 0, W, H);
    sCtx.translate(W / 2 + panX, H / 2 + panY);
    sCtx.rotate((rotationDeg * Math.PI) / 180);
    
    const effectiveScale = baseScale * zoom;
    sCtx.scale(effectiveScale, effectiveScale);
    sCtx.drawImage(loadedImg, -loadedImg.width / 2, -loadedImg.height / 2);
    sCtx.restore();

    // Dither processing
    const imgData = sCtx.getImageData(0, 0, W, H);
    const d = imgData.data;
    const bright = parseInt(brightSlider.value);
    const contrast = parseFloat(contrastSlider.value);
    const mode = ditherMode.value;

    const gray = new Float32Array(W * H);
    for (let i = 0; i < W * H; i++) {
      let r = d[i * 4];
      let g = d[i * 4 + 1];
      let b = d[i * 4 + 2];
      let lum = 0.299 * r + 0.587 * g + 0.114 * b;
      lum = (lum - 128) * contrast + 128 + bright;
      gray[i] = Math.max(0, Math.min(255, lum));
    }

    if (mode === 'floyd') {
      for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
          let idx = y * W + x;
          let oldVal = gray[idx];
          let newVal = oldVal > 128 ? 255 : 0;
          gray[idx] = newVal;
          let err = oldVal - newVal;
          if (x + 1 < W) gray[idx + 1] += err * (7 / 16);
          if (x - 1 >= 0 && y + 1 < H) gray[idx + W - 1] += err * (3 / 16);
          if (y + 1 < H) gray[idx + W] += err * (5 / 16);
          if (x + 1 < W && y + 1 < H) gray[idx + W + 1] += err * (1 / 16);
        }
      }
    } else if (mode === 'atkinson') {
      for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
          let idx = y * W + x;
          let oldVal = gray[idx];
          let newVal = oldVal > 128 ? 255 : 0;
          gray[idx] = newVal;
          let err = (oldVal - newVal) >> 3;
          if (x + 1 < W) gray[idx + 1] += err;
          if (x + 2 < W) gray[idx + 2] += err;
          if (x - 1 >= 0 && y + 1 < H) gray[idx + W - 1] += err;
          if (y + 1 < H) gray[idx + W] += err;
          if (x + 1 < W && y + 1 < H) gray[idx + W + 1] += err;
          if (y + 2 < H) gray[idx + W * 2] += err;
        }
      }
    } else {
      // Clean Threshold
      for (let i = 0; i < W * H; i++) {
        gray[i] = gray[i] > 128 ? 255 : 0;
      }
    }

    // Write to visible canvas
    const outData = ctx.createImageData(W, H);
    for (let i = 0; i < W * H; i++) {
      let val = gray[i] > 128 ? 255 : 0;
      if (isInverted) val = 255 - val;
      outData.data[i * 4] = val;
      outData.data[i * 4 + 1] = val;
      outData.data[i * 4 + 2] = val;
      outData.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(outData, 0, 0);

    // Real-Time Live Sync to OLED!
    triggerLiveSync();
  }

  // Pack 128x64 pixels into 1024 bytes (16 bytes/row x 64 rows)
  function packCanvasHex() {
    const imgData = ctx.getImageData(0, 0, 128, 64).data;
    let hex = "";
    for (let y = 0; y < 64; y++) {
      for (let byteX = 0; byteX < 16; byteX++) {
        let b = 0;
        for (let bit = 0; bit < 8; bit++) {
          let x = byteX * 8 + bit;
          let i = (y * 128 + x) * 4;
          if (imgData[i] > 128) {
            b |= (1 << (7 - bit));
          }
        }
        let h = b.toString(16);
        if (h.length < 2) h = "0" + h;
        hex += h;
      }
    }
    return hex;
  }

  // Real-time Live OLED Sync sender
  function triggerLiveSync() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    const now = Date.now();
    if (isSendingLive || (now - lastLiveSend < LIVE_INTERVAL_MS)) {
      pendingLiveFrame = true;
      return;
    }
    sendLiveFrame();
  }

  function sendLiveFrame() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    isSendingLive = true;
    lastLiveSend = Date.now();
    pendingLiveFrame = false;

    const hex = packCanvasHex();
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + hex + '&save=0'
    })
    .catch(() => {})
    .finally(() => {
      isSendingLive = false;
      if (pendingLiveFrame && liveSyncCheck.checked) {
        setTimeout(sendLiveFrame, 30);
      }
    });
  }

  // Permanent Save button
  sendBtn.addEventListener('click', () => {
    sendBtn.disabled = true;
    sendBtn.innerText = '⏳ Saving...';
    
    const hex = packCanvasHex();
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + hex + '&save=1'
    })
    .then(res => res.json())
    .then(data => {
      sendBtn.disabled = false;
      sendBtn.innerText = '💾 Save to DeskBuddy (Permanent)';
      if (data.status === 'ok') {
        showToast('✅ Saved Permanently to DeskBuddy!', '#00C853');
      } else {
        showToast('Save failed: ' + (data.msg || 'error'), '#d32f2f');
      }
    })
    .catch(() => {
      sendBtn.disabled = false;
      sendBtn.innerText = '💾 Save to DeskBuddy (Permanent)';
      showToast('Saved! Check DeskBuddy screen', '#00C853');
    });
  });
</script>
</body>
</html>
)rawliteral";
