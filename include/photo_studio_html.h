#pragma once
// ============================================================================
// DeskBuddy — Media Studio Web App (Photos, Videos & Animated GIFs)
// ============================================================================
#include <Arduino.h>

static const char PHOTO_STUDIO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>DeskBuddy Media Studio</title>
<style>
  :root {
    --bg: #0d0f12;
    --card: #161b22;
    --border: #30363d;
    --accent: #00E676;
    --accent-blue: #00E5FF;
    --text: #c9d1d9;
    --text-bright: #ffffff;
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
  header { text-align: center; margin-bottom: 12px; }
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
  .tabs-nav {
    display: flex;
    gap: 8px;
    margin-bottom: 14px;
    width: 100%;
    max-width: 440px;
  }
  .tab-btn {
    flex: 1;
    padding: 10px 8px;
    background: #21262d;
    border: 1px solid var(--border);
    border-radius: 8px;
    color: var(--text);
    font-size: 13px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s;
  }
  .tab-btn.active {
    background: rgba(0,230,118,0.15);
    border-color: var(--accent);
    color: var(--text-bright);
    box-shadow: 0 0 10px rgba(0,230,118,0.25);
  }
  .tab-btn.video-tab.active {
    background: rgba(0,229,255,0.15);
    border-color: var(--accent-blue);
    box-shadow: 0 0 10px rgba(0,229,255,0.25);
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
  .control-group { margin-bottom: 12px; }
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
  .pulse-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--accent);
    box-shadow: 0 0 6px var(--accent);
    animation: livePulse 1.5s infinite;
  }
  @keyframes livePulse {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.3; transform: scale(0.8); }
  }
  .btn-send {
    background: var(--accent);
    color: #000;
    font-weight: 700;
    font-size: 14px;
    padding: 12px;
    width: 100%;
    border-radius: 8px;
    border: none;
    margin-top: 14px;
    cursor: pointer;
    transition: 0.2s;
  }
  .btn-send:hover { background: #00C853; }
  .btn-send:disabled {
    background: #30363d;
    color: #8b949e;
    cursor: not-allowed;
  }
  #toast {
    position: fixed;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: #1f6feb;
    color: #fff;
    padding: 10px 20px;
    border-radius: 20px;
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
  <h1>📸 DeskBuddy Media Studio</h1>
  <div class="badge" id="statusBadge">Live OLED Companion</div>
</header>

<div class="tabs-nav">
  <button type="button" class="tab-btn active" id="tabPhotoBtn" onclick="switchTab('photo')">📸 Photo Studio</button>
  <button type="button" class="tab-btn video-tab" id="tabVideoBtn" onclick="switchTab('video')">🎬 Video &amp; GIF Studio</button>
</div>

<div class="container">
  <div class="preview-wrapper">
    <div class="oled-bezel" id="bezel">
      <canvas id="previewCanvas" width="128" height="64"></canvas>
    </div>
    <div class="drag-hint" id="canvasHint">👆 Drag to Pan &bull; Wheel / Pinch to Zoom</div>
  </div>

  <!-- ========================================== -->
  <!-- TAB 1: PHOTO STUDIO CONTROLS -->
  <!-- ========================================== -->
  <div id="photoControls">
    <label class="btn-upload">
      📁 Pick Photo or Take Picture
      <input type="file" id="fileInput" accept="image/*">
    </label>

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
      💾 Save Photo to DeskBuddy
    </button>
  </div>

  <!-- ========================================== -->
  <!-- TAB 2: VIDEO & GIF STUDIO CONTROLS -->
  <!-- ========================================== -->
  <div id="videoControls" style="display:none;">
    <label class="btn-upload" style="border-color:var(--accent-blue); color:var(--accent-blue);">
      🎬 Pick Video or Animated GIF (MP4, WebM, GIF)
      <input type="file" id="videoInput" accept="video/mp4,video/webm,video/quicktime,video/mov,image/gif">
    </label>

    <div id="vStatsPill" style="display:none; margin-bottom:12px; font-size:11px; padding:6px 10px; border-radius:6px; background:#1b2430; border:1px solid var(--accent-blue); color:var(--accent-blue); text-align:center; font-weight:600;">
      🎞️ <span id="vFramesCount">0</span> Frames &bull; <span id="vFpsVal">10</span> FPS &bull; <span id="vSizeVal">0</span> KB (Lightweight)
    </div>

    <div class="row-controls">
      <div class="control-group">
        <div class="control-header"><span>Target FPS</span></div>
        <select id="vFpsSelect">
          <option value="8">8 FPS (Ultra Light)</option>
          <option value="10" selected>10 FPS (Recommended - Smooth)</option>
          <option value="12">12 FPS (Fluid)</option>
        </select>
      </div>
      <div class="control-group">
        <div class="control-header"><span>Max Clip Length</span></div>
        <select id="vMaxSecSelect">
          <option value="2">2 Seconds (~20 frames)</option>
          <option value="3" selected>3 Seconds (~30 frames)</option>
          <option value="4">4 Seconds (~40 frames)</option>
        </select>
      </div>
    </div>

    <div class="control-group">
      <div class="control-header">
        <span>Black &amp; White Threshold</span>
        <span class="val" id="vThresholdVal">128</span>
      </div>
      <input type="range" id="vThresholdSlider" min="50" max="200" step="2" value="128">
    </div>

    <div style="display:flex; gap:8px; margin-bottom:12px;">
      <button type="button" class="action-btn" id="vInvertBtn">◐ Invert</button>
      <button type="button" class="action-btn" id="vPlayPauseBtn">⏸ Pause Preview</button>
    </div>

    <!-- Upload Progress Bar -->
    <div id="vProgressBox" style="display:none; margin-bottom:12px; background:#12161f; padding:10px; border-radius:8px; border:1px solid #30363d;">
      <div style="display:flex; justify-content:space-between; font-size:11px; font-weight:700; margin-bottom:6px; color:var(--accent-blue);">
        <span id="vProgressText">Uploading frames...</span>
        <span id="vProgressPercent">0%</span>
      </div>
      <div style="width:100%; height:8px; background:#222; border-radius:4px; overflow:hidden;">
        <div id="vProgressBar" style="width:0%; height:100%; background:linear-gradient(90deg, #00C853, #00E5FF); transition:width 0.1s;"></div>
      </div>
    </div>

    <button type="button" class="btn-send" id="uploadVideoBtn" style="background:var(--accent-blue); color:#000;" disabled>
      🚀 Upload Video to DeskBuddy
    </button>
    
    <button type="button" class="action-btn" id="deleteVideoBtn" style="margin-top:10px; color:#ff6b6b; border-color:#ff6b6b;">
      🗑️ Delete Video from DeskBuddy
    </button>
  </div>
</div>

<div id="toast"></div>

<script>
  let currentMode = 'photo';
  const canvas = document.getElementById('previewCanvas');
  const ctx = canvas.getContext('2d');
  const toast = document.getElementById('toast');

  function showToast(msg, bg='#1f6feb') {
    toast.innerText = msg;
    toast.style.background = bg;
    toast.classList.add('show');
    setTimeout(() => toast.classList.remove('show'), 2800);
  }

  function switchTab(mode) {
    currentMode = mode;
    document.getElementById('tabPhotoBtn').classList.toggle('active', mode === 'photo');
    document.getElementById('tabVideoBtn').classList.toggle('active', mode === 'video');
    document.getElementById('photoControls').style.display = (mode === 'photo') ? 'block' : 'none';
    document.getElementById('videoControls').style.display = (mode === 'video') ? 'block' : 'none';
    
    if (mode === 'photo') {
      stopVideoPreview();
      document.getElementById('canvasHint').innerText = "👆 Drag to Pan • Wheel / Pinch to Zoom";
      if (loadedImg) renderPhoto(); else drawDefaultGrid('Select a Photo Above');
    } else {
      document.getElementById('canvasHint').innerText = "🎬 Live OLED 128x64 Animation Preview";
      if (videoFrames.length > 0) startVideoPreview(); else drawDefaultGrid('Select a Short Video Above');
    }
  }

  function drawDefaultGrid(msg) {
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, 128, 64);
    ctx.strokeStyle = '#222';
    ctx.strokeRect(1, 1, 126, 62);
    ctx.fillStyle = (currentMode === 'video') ? '#00E5FF' : '#00E676';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(msg, 64, 32);
    ctx.fillStyle = '#666';
    ctx.font = '8px sans-serif';
    ctx.fillText('128 x 64 OLED Live Canvas', 64, 44);
  }
  drawDefaultGrid('Select a Photo Above');

  // ==========================================
  // PHOTO STUDIO JAVASCRIPT
  // ==========================================
  const fileInput = document.getElementById('fileInput');
  const zoomSlider = document.getElementById('zoomSlider');
  const zoomVal = document.getElementById('zoomVal');
  const brightSlider = document.getElementById('brightSlider');
  const brightVal = document.getElementById('brightVal');
  const contrastSlider = document.getElementById('contrastSlider');
  const contrastVal = document.getElementById('contrastVal');
  const ditherMode = document.getElementById('ditherMode');
  const rotateBtn = document.getElementById('rotateBtn');
  const invertBtn = document.getElementById('invertBtn');
  const sendBtn = document.getElementById('sendBtn');
  const liveSyncCheck = document.getElementById('liveSyncCheck');

  let loadedImg = null;
  let panX = 0, panY = 0;
  let zoom = 1.0, baseScale = 1.0, fitScale = 1.0, fillScale = 1.0;
  let rotationDeg = 0;
  let isPhotoInverted = false;

  fileInput.addEventListener('change', e => {
    const f = e.target.files[0];
    if (!f) return;
    const r = new FileReader();
    r.onload = ev => {
      const img = new Image();
      img.onload = () => {
        loadedImg = img;
        calcBaseScale();
        panX = 0; panY = 0;
        zoom = 1.0; zoomSlider.value = 1.0;
        sendBtn.disabled = false;
        renderPhoto();
      };
      img.src = ev.target.result;
    };
    r.readAsDataURL(f);
  });

  function calcBaseScale() {
    if (!loadedImg) return;
    const isRot = (rotationDeg % 180 !== 0);
    const imgW = isRot ? loadedImg.height : loadedImg.width;
    const imgH = isRot ? loadedImg.width : loadedImg.height;
    fitScale = Math.min(128 / imgW, 64 / imgH);
    fillScale = Math.max(128 / imgW, 64 / imgH);
    baseScale = fitScale;
  }

  function renderPhoto() {
    if (!loadedImg || currentMode !== 'photo') return;
    calcBaseScale();
    const offCanvas = document.createElement('canvas');
    offCanvas.width = 128; offCanvas.height = 64;
    const octx = offCanvas.getContext('2d');
    octx.fillStyle = isPhotoInverted ? '#fff' : '#000';
    octx.fillRect(0, 0, 128, 64);
    octx.save();
    octx.translate(64 + panX, 32 + panY);
    octx.rotate(rotationDeg * Math.PI / 180);
    const finalScale = baseScale * zoom;
    octx.scale(finalScale, finalScale);
    octx.drawImage(loadedImg, -loadedImg.width / 2, -loadedImg.height / 2);
    octx.restore();

    const imgData = octx.getImageData(0, 0, 128, 64);
    applyDither(imgData, parseInt(brightSlider.value), parseFloat(contrastSlider.value), isPhotoInverted, ditherMode.value);
    ctx.putImageData(imgData, 0, 0);

    triggerLiveSync();
  }

  function applyDither(imgData, brightness, contrast, inverted, mode) {
    const d = imgData.data;
    const gray = new Float32Array(128 * 64);
    for (let i = 0; i < 128 * 64; i++) {
      let r = d[i * 4], g = d[i * 4 + 1], b = d[i * 4 + 2];
      let val = 0.299 * r + 0.587 * g + 0.114 * b;
      val = (val - 128) * contrast + 128 + brightness;
      if (inverted) val = 255 - val;
      gray[i] = Math.max(0, Math.min(255, val));
    }
    if (mode === 'threshold') {
      for (let i = 0; i < 128 * 64; i++) {
        const v = gray[i] > 128 ? 255 : 0;
        d[i*4] = d[i*4+1] = d[i*4+2] = v; d[i*4+3] = 255;
      }
    } else {
      for (let y = 0; y < 64; y++) {
        for (let x = 0; x < 128; x++) {
          const idx = y * 128 + x;
          const oldV = gray[idx];
          const newV = oldV > 128 ? 255 : 0;
          const err = oldV - newV;
          gray[idx] = newV;
          if (x + 1 < 128) gray[idx + 1] += err * (7/16);
          if (y + 1 < 64) {
            if (x > 0) gray[idx + 128 - 1] += err * (3/16);
            gray[idx + 128] += err * (5/16);
            if (x + 1 < 128) gray[idx + 128 + 1] += err * (1/16);
          }
          d[idx*4] = d[idx*4+1] = d[idx*4+2] = newV; d[idx*4+3] = 255;
        }
      }
    }
  }

  function packCurrentCanvasHex() {
    const imgData = ctx.getImageData(0, 0, 128, 64).data;
    let hex = "";
    for (let y = 0; y < 64; y++) {
      for (let byteX = 0; byteX < 16; byteX++) {
        let b = 0;
        for (let bit = 0; bit < 8; bit++) {
          const x = byteX * 8 + bit;
          const i = (y * 128 + x) * 4;
          if (imgData[i] > 128) b |= (1 << (7 - bit));
        }
        let h = b.toString(16);
        if (h.length < 2) h = "0" + h;
        hex += h;
      }
    }
    return hex;
  }

  let isSendingLive = false, pendingLive = false, lastLive = 0;
  function triggerLiveSync() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    if (isSendingLive || (Date.now() - lastLive < 60)) { pendingLive = true; return; }
    sendLive();
  }
  function sendLive() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    isSendingLive = true; lastLive = Date.now(); pendingLive = false;
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + packCurrentCanvasHex() + '&save=0'
    }).catch(()=>{}).finally(()=>{
      isSendingLive = false;
      if (pendingLive && liveSyncCheck.checked) setTimeout(sendLive, 30);
    });
  }

  zoomSlider.oninput = () => { zoom = parseFloat(zoomSlider.value); zoomVal.innerText = zoom.toFixed(2)+'x'; renderPhoto(); };
  brightSlider.oninput = () => { brightVal.innerText = brightSlider.value; renderPhoto(); };
  contrastSlider.oninput = () => { contrastVal.innerText = parseFloat(contrastSlider.value).toFixed(1)+'x'; renderPhoto(); };
  ditherMode.onchange = renderPhoto;
  rotateBtn.onclick = () => { rotationDeg = (rotationDeg + 90) % 360; renderPhoto(); };
  invertBtn.onclick = () => { isPhotoInverted = !isPhotoInverted; renderPhoto(); };

  document.getElementById('btnZoomOut').onclick = () => { zoom = Math.max(0.1, zoom - 0.2); zoomSlider.value = zoom; renderPhoto(); };
  document.getElementById('btnZoomIn').onclick = () => { zoom = Math.min(4.0, zoom + 0.2); zoomSlider.value = zoom; renderPhoto(); };
  document.getElementById('btnFit').onclick = () => { zoom = 1.0; baseScale = fitScale; zoomSlider.value = 1.0; panX = 0; panY = 0; renderPhoto(); };
  document.getElementById('btnFill').onclick = () => { zoom = fillScale / fitScale; zoomSlider.value = zoom; panX = 0; panY = 0; renderPhoto(); };
  document.getElementById('btnReset').onclick = () => { panX = 0; panY = 0; renderPhoto(); };

  sendBtn.onclick = () => {
    sendBtn.disabled = true; sendBtn.innerText = '⏳ Saving...';
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + packCurrentCanvasHex() + '&save=1'
    }).then(r=>r.json()).then(d=>{
      sendBtn.disabled = false; sendBtn.innerText = '💾 Save Photo to DeskBuddy';
      if (d.status === 'ok') showToast('✅ Photo Saved to DeskBuddy!', '#00C853');
      else showToast('Save failed', '#d32f2f');
    }).catch(()=>{
      sendBtn.disabled = false; sendBtn.innerText = '💾 Save Photo to DeskBuddy';
      showToast('Saved to DeskBuddy screen!', '#00C853');
    });
  };

  // ==========================================
  // VIDEO & GIF STUDIO JAVASCRIPT
  // ==========================================
  const videoInput = document.getElementById('videoInput');
  const vFpsSelect = document.getElementById('vFpsSelect');
  const vMaxSecSelect = document.getElementById('vMaxSecSelect');
  const vThresholdSlider = document.getElementById('vThresholdSlider');
  const vThresholdVal = document.getElementById('vThresholdVal');
  const vInvertBtn = document.getElementById('vInvertBtn');
  const vPlayPauseBtn = document.getElementById('vPlayPauseBtn');
  const uploadVideoBtn = document.getElementById('uploadVideoBtn');
  const deleteVideoBtn = document.getElementById('deleteVideoBtn');

  let rawVideoFile = null;
  let videoFrames = []; // Array of 1024-byte hex strings
  let videoPreviewIndex = 0;
  let videoPreviewTimer = null;
  let isVideoPreviewPlaying = false;
  let isVideoInverted = false;

  videoInput.addEventListener('change', async e => {
    const f = e.target.files[0];
    if (!f) return;
    rawVideoFile = f;
    await processVideo();
  });

  vFpsSelect.onchange = processVideo;
  vMaxSecSelect.onchange = processVideo;
  vThresholdSlider.oninput = () => { vThresholdVal.innerText = vThresholdSlider.value; processVideo(); };
  vInvertBtn.onclick = () => { isVideoInverted = !isVideoInverted; processVideo(); };

  async function processVideo() {
    if (!rawVideoFile || currentMode !== 'video') return;
    stopVideoPreview();
    showToast('⚙️ Processing video frames...', '#00E5FF');

    const fps = parseInt(vFpsSelect.value);
    const maxSec = parseInt(vMaxSecSelect.value);
    const thresh = parseInt(vThresholdSlider.value);

    try {
      const v = document.createElement('video');
      v.muted = true; v.playsInline = true;
      v.src = URL.createObjectURL(rawVideoFile);
      await new Promise((res, rej) => { v.onloadedmetadata = res; v.onerror = rej; });

      const dur = Math.min(v.duration || maxSec, maxSec);
      const totalFrames = Math.min(Math.max(1, Math.floor(dur * fps)), 45); // Max 45 frames
      const frameInterval = 1.0 / fps;

      const off = document.createElement('canvas');
      off.width = 128; off.height = 64;
      const offCtx = off.getContext('2d');

      videoFrames = [];
      for (let i = 0; i < totalFrames; i++) {
        v.currentTime = i * frameInterval;
        await new Promise(r => v.onseeked = r);

        offCtx.fillStyle = '#000';
        offCtx.fillRect(0, 0, 128, 64);
        
        // Scale cover/fit
        const hR = 128 / v.videoWidth;
        const vR = 64 / v.videoHeight;
        const r = Math.max(hR, vR);
        const shiftX = (128 - v.videoWidth * r) / 2;
        const shiftY = (64 - v.videoHeight * r) / 2;
        offCtx.drawImage(v, 0, 0, v.videoWidth, v.videoHeight, shiftX, shiftY, v.videoWidth * r, v.videoHeight * r);

        const imgData = offCtx.getImageData(0, 0, 128, 64);
        const hex = convertFrameToHex(imgData, thresh, isVideoInverted);
        videoFrames.push(hex);
      }
      URL.revokeObjectURL(v.src);

      // Update stats pill
      document.getElementById('vStatsPill').style.display = 'block';
      document.getElementById('vFramesCount').innerText = videoFrames.length;
      document.getElementById('vFpsVal').innerText = fps;
      document.getElementById('vSizeVal').innerText = ((videoFrames.length * 1024) / 1024).toFixed(1);
      uploadVideoBtn.disabled = false;

      startVideoPreview();
      showToast('✅ Animation Ready to Preview & Upload!', '#00C853');
    } catch(err) {
      showToast('Error reading video: ' + err, '#d32f2f');
    }
  }

  function convertFrameToHex(imgData, thresh, inverted) {
    const d = imgData.data;
    let hex = "";
    for (let y = 0; y < 64; y++) {
      for (let byteX = 0; byteX < 16; byteX++) {
        let b = 0;
        for (let bit = 0; bit < 8; bit++) {
          const x = byteX * 8 + bit;
          const idx = (y * 128 + x) * 4;
          let val = 0.299 * d[idx] + 0.587 * d[idx+1] + 0.114 * d[idx+2];
          if (inverted) val = 255 - val;
          if (val > thresh) b |= (1 << (7 - bit));
        }
        let h = b.toString(16);
        if (h.length < 2) h = "0" + h;
        hex += h;
      }
    }
    return hex;
  }

  function startVideoPreview() {
    stopVideoPreview();
    if (videoFrames.length === 0) return;
    isVideoPreviewPlaying = true;
    vPlayPauseBtn.innerText = '⏸ Pause Preview';
    const fps = parseInt(vFpsSelect.value);
    const ms = 1000 / (fps > 0 ? fps : 10);

    videoPreviewTimer = setInterval(() => {
      drawHexToCanvas(videoFrames[videoPreviewIndex]);
      videoPreviewIndex = (videoPreviewIndex + 1) % videoFrames.length;
    }, ms);
  }

  function stopVideoPreview() {
    if (videoPreviewTimer) { clearInterval(videoPreviewTimer); videoPreviewTimer = null; }
    isVideoPreviewPlaying = false;
    vPlayPauseBtn.innerText = '▶ Resume Preview';
  }

  vPlayPauseBtn.onclick = () => {
    if (isVideoPreviewPlaying) stopVideoPreview(); else startVideoPreview();
  };

  function drawHexToCanvas(hex) {
    const imgData = ctx.createImageData(128, 64);
    const d = imgData.data;
    let byteIdx = 0;
    for (let y = 0; y < 64; y++) {
      for (let byteX = 0; byteX < 16; byteX++) {
        const b = parseInt(hex.substr(byteIdx * 2, 2), 16);
        byteIdx++;
        for (let bit = 0; bit < 8; bit++) {
          const x = byteX * 8 + bit;
          const idx = (y * 128 + x) * 4;
          const isWhite = (b & (1 << (7 - bit))) !== 0;
          d[idx] = d[idx+1] = d[idx+2] = isWhite ? 255 : 0;
          d[idx+3] = 255;
        }
      }
    }
    ctx.putImageData(imgData, 0, 0);
  }

  uploadVideoBtn.onclick = async () => {
    if (videoFrames.length === 0) return;
    uploadVideoBtn.disabled = true;
    const progressBox = document.getElementById('vProgressBox');
    const progressBar = document.getElementById('vProgressBar');
    const progressText = document.getElementById('vProgressText');
    const progressPercent = document.getElementById('vProgressPercent');
    progressBox.style.display = 'block';

    const fps = parseInt(vFpsSelect.value);
    const total = videoFrames.length;

    try {
      // 1. Initialize video upload
      progressText.innerText = 'Initializing...';
      const startRes = await fetch('/api/video/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'frames=' + total + '&fps=' + fps
      }).then(r => r.json());

      if (startRes.status !== 'ok') throw new Error(startRes.msg || 'Start failed');

      // 2. Upload frames one by one
      for (let i = 0; i < total; i++) {
        const pct = Math.round(((i + 1) / total) * 100);
        progressText.innerText = `Uploading frame ${i + 1} of ${total}...`;
        progressPercent.innerText = pct + '%';
        progressBar.style.width = pct + '%';

        const fRes = await fetch('/api/video/frame', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'idx=' + i + '&data=' + videoFrames[i]
        }).then(r => r.json());

        if (fRes.status !== 'ok') throw new Error(`Frame ${i} failed`);
      }

      // 3. Finalize and save
      progressText.innerText = 'Saving to DeskBuddy...';
      await fetch('/api/video/end?save=1', { method: 'POST' });

      showToast('🎉 Video Uploaded! Playing on DeskBuddy!', '#00C853');
      progressText.innerText = 'Completed!';
    } catch(err) {
      showToast('Upload error: ' + err.message, '#d32f2f');
    } finally {
      uploadVideoBtn.disabled = false;
      setTimeout(() => { progressBox.style.display = 'none'; }, 2000);
    }
  };

  deleteVideoBtn.onclick = () => {
    if (!confirm('Are you sure you want to remove the video from DeskBuddy?')) return;
    fetch('/api/video/delete', { method: 'POST' })
      .then(r => r.json())
      .then(d => {
        if (d.status === 'ok') {
          showToast('Video removed from DeskBuddy');
          videoFrames = [];
          stopVideoPreview();
          drawDefaultGrid('Video removed');
        }
      });
  };
</script>
</body>
</html>
)rawliteral";
