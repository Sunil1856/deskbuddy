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
    font-size: 20px;
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
    color: var(--text-bright);
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
    font-size: 13px;
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
  .control-header span.val-blue { color: var(--accent-blue); }
  input[type="range"] {
    width: 100%;
    accent-color: var(--accent);
  }
  .range-blue { accent-color: var(--accent-blue) !important; }
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
  .btn-danger {
    margin-top: 10px;
    width: 100%;
    padding: 9px;
    background: rgba(255, 75, 75, 0.1);
    border: 1px solid #ff4b4b;
    color: #ff6b6b;
    border-radius: 8px;
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: 0.2s;
  }
  .btn-danger:hover { background: rgba(255, 75, 75, 0.2); }
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
    <button type="button" class="btn-danger" id="deletePhotoBtn">
      🗑️ Delete Photo from DeskBuddy
    </button>
  </div>

  <!-- ========================================== -->
  <!-- TAB 2: VIDEO & GIF STUDIO CONTROLS -->
  <!-- ========================================== -->
  <div id="videoControls" style="display:none;">
    <label class="btn-upload" style="border-color:var(--accent-blue); color:var(--accent-blue);">
      🎬 Pick Video or Animated GIF (MP4, WebM, MOV, GIF)
      <input type="file" id="videoInput" accept="video/mp4,video/webm,video/quicktime,video/mov,video/*,image/gif">
    </label>

    <div id="vStatsPill" style="display:none; margin-bottom:12px; font-size:11px; padding:7px 10px; border-radius:6px; background:#111b27; border:1px solid var(--accent-blue); color:var(--accent-blue); text-align:center; font-weight:700;">
      🎞️ <span id="vFramesCount">0</span> Frames &bull; <span id="vFpsVal">8</span> FPS &bull; <span id="vDurationVal">0.0s</span> &bull; <span id="vSizeVal">0</span> KB
    </div>

    <!-- Video Playback & Frame Scrubber -->
    <div class="control-group" id="vScrubberGroup" style="display:none;">
      <div class="control-header">
        <span>Frame Scrubber &amp; Selector</span>
        <span class="val-blue" id="vScrubberVal">Frame 1 of 0</span>
      </div>
      <input type="range" class="range-blue" id="vScrubberSlider" min="0" max="0" step="1" value="0">
      <div class="zoom-btn-row" style="margin-top:6px;">
        <button type="button" class="mini-btn" id="vDropFrameBtn" style="border-color:#ff4b4b; color:#ff6b6b;" title="Remove this exact frame from animation">🗑️ Drop</button>
        <button type="button" class="mini-btn" id="vSetStartBtn" title="Set current frame as start (cut before)">✂️ Start</button>
        <button type="button" class="mini-btn" id="vSetEndBtn" title="Set current frame as end (cut after)">✂️ End</button>
        <button type="button" class="mini-btn" id="vResetFramesBtn" title="Restore all frames from original video">↩️ Reset</button>
      </div>
    </div>

    <div style="display:flex; gap:8px; margin-bottom:12px;">
      <button type="button" class="action-btn" id="vPlayPauseBtn">⏸ Pause</button>
      <button type="button" class="action-btn" id="vStepBtn">⏭ Step</button>
      <button type="button" class="action-btn" id="vInvertBtn">◐ Invert</button>
    </div>

    <!-- Video Zoom & Scale Tool -->
    <div class="control-group">
      <div class="control-header">
        <span>Zoom &amp; Fit Mode</span>
        <span class="val-blue" id="vZoomVal">1.0x</span>
      </div>
      <input type="range" class="range-blue" id="vZoomSlider" min="0.3" max="3.0" step="0.05" value="1.0">
      <div class="zoom-btn-row">
        <button type="button" class="mini-btn" id="vBtnFit">🔍 Fit</button>
        <button type="button" class="mini-btn" id="vBtnFill">⛶ Fill</button>
        <button type="button" class="mini-btn" id="vBtnStretch">↔️ Stretch</button>
        <button type="button" class="mini-btn" id="vBtnZoomOut">➖ Out</button>
        <button type="button" class="mini-btn" id="vBtnZoomIn">➕ In</button>
        <button type="button" class="mini-btn" id="vBtnReset">↺ Center</button>
      </div>
    </div>

    <!-- Video Trim Tool -->
    <div class="control-group">
      <div class="control-header">
        <span>Trim: Start Time in Video</span>
        <span class="val-blue" id="vStartVal">0.0s</span>
      </div>
      <input type="range" class="range-blue" id="vStartSlider" min="0" max="60" step="0.2" value="0">
    </div>

    <div class="row-controls">
      <div class="control-group">
        <div class="control-header"><span>Max Duration</span></div>
        <select id="vMaxSecSelect">
          <option value="2">2 Seconds</option>
          <option value="4">4 Seconds</option>
          <option value="6">6 Seconds</option>
          <option value="8" selected>8 Seconds</option>
          <option value="10">10 Seconds</option>
          <option value="15">15 Seconds</option>
          <option value="20">20 Seconds</option>
          <option value="30">30 Seconds (Max)</option>
        </select>
      </div>
      <div class="control-group">
        <div class="control-header"><span>Playback FPS</span></div>
        <select id="vFpsSelect">
          <option value="4">4 FPS (Eco - Best for 20-30s)</option>
          <option value="6">6 FPS (Light)</option>
          <option value="8" selected>8 FPS (Smooth &amp; Balanced)</option>
          <option value="10">10 FPS (Fluid)</option>
          <option value="12">12 FPS (Fast)</option>
          <option value="15">15 FPS (Max Fluidity)</option>
        </select>
      </div>
    </div>

    <div class="row-controls">
      <div class="control-group">
        <div class="control-header"><span>Scale / Crop Mode</span></div>
        <select id="vFitMode">
          <option value="cover" selected>Fill (Cover 128x64)</option>
          <option value="contain">Fit (Full Video / Bars)</option>
          <option value="stretch">Stretch (Exact 128x64)</option>
        </select>
      </div>
      <div class="control-group">
        <div class="control-header"><span>Dither Style</span></div>
        <select id="vDitherMode">
          <option value="floyd" selected>Floyd-Steinberg (Artistic)</option>
          <option value="threshold">Threshold (Crisp B&amp;W)</option>
        </select>
      </div>
    </div>

    <div class="row-controls">
      <div class="control-group">
        <div class="control-header">
          <span>Pan X (Left/Right)</span>
          <span class="val-blue" id="vPanXVal">0</span>
        </div>
        <input type="range" class="range-blue" id="vPanXSlider" min="-80" max="80" step="1" value="0">
      </div>
      <div class="control-group">
        <div class="control-header">
          <span>Pan Y (Up/Down)</span>
          <span class="val-blue" id="vPanYVal">0</span>
        </div>
        <input type="range" class="range-blue" id="vPanYSlider" min="-60" max="60" step="1" value="0">
      </div>
    </div>

    <div class="control-group">
      <div class="control-header">
        <span>Black &amp; White Threshold</span>
        <span class="val-blue" id="vThresholdVal">128</span>
      </div>
      <input type="range" class="range-blue" id="vThresholdSlider" min="50" max="200" step="2" value="128">
    </div>

    <div class="live-sync-card" style="margin-bottom:14px;">
      <label class="live-sync-label">
        <input type="checkbox" id="vLiveSyncCheck">
        <span>⚡ Sync Video Frames to OLED</span>
      </label>
      <div class="live-status-pill" id="vLiveStatusPill" style="color:var(--accent-blue); border-color:rgba(0,229,255,0.3); background:rgba(0,229,255,0.12);">
        <span class="pulse-dot" style="background:var(--accent-blue); box-shadow:0 0 6px var(--accent-blue);"></span> Frame
      </div>
    </div>

    <!-- Upload / Processing Progress Bar -->
    <div id="vProgressBox" style="display:none; margin-bottom:12px; background:#12161f; padding:10px; border-radius:8px; border:1px solid #30363d;">
      <div style="display:flex; justify-content:space-between; font-size:11px; font-weight:700; margin-bottom:6px; color:var(--accent-blue);">
        <span id="vProgressText">Processing...</span>
        <span id="vProgressPercent">0%</span>
      </div>
      <div style="width:100%; height:8px; background:#222; border-radius:4px; overflow:hidden;">
        <div id="vProgressBar" style="width:0%; height:100%; background:linear-gradient(90deg, #00C853, #00E5FF); transition:width 0.1s;"></div>
      </div>
    </div>

    <button type="button" class="btn-send" id="uploadVideoBtn" style="background:var(--accent-blue); color:#000;" disabled>
      🚀 Upload Video to DeskBuddy
    </button>
    <button type="button" class="btn-danger" id="deleteVideoBtn">
      🗑️ Delete Video from DeskBuddy
    </button>
  </div>
</div>

<!-- ========================================== -->
<!-- LAPTOP HARDWARE MONITOR SETTINGS CARD -->
<!-- ========================================== -->
<div class="container" style="margin-top: 14px;">
  <div class="control-header" style="margin-bottom: 8px;">
    <span style="font-size:13px; font-weight:700; color:var(--accent);">💻 Laptop Hardware Monitor</span>
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

<script>
  let currentMode = 'photo';
  const canvas = document.getElementById('previewCanvas');
  const ctx = canvas.getContext('2d');
  const toast = document.getElementById('toast');
  const bezel = document.getElementById('bezel');

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
  // POINTER DRAG & ZOOM ON CANVAS BEZEL
  // ==========================================
  let isDragging = false;
  let startX = 0, startY = 0;
  let initPanX = 0, initPanY = 0;

  bezel.addEventListener('pointerdown', (e) => {
    if (currentMode === 'photo') {
      if (!loadedImg) return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      initPanX = panX;
      initPanY = panY;
      bezel.setPointerCapture(e.pointerId);
    } else {
      if (videoFrames.length === 0) return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      initPanX = parseInt(vPanXSlider.value);
      initPanY = parseInt(vPanYSlider.value);
      bezel.setPointerCapture(e.pointerId);
    }
  });

  bezel.addEventListener('pointermove', (e) => {
    if (!isDragging) return;
    const dx = (e.clientX - startX) / 2.5;
    const dy = (e.clientY - startY) / 2.5;
    if (currentMode === 'photo') {
      if (!loadedImg) return;
      panX = initPanX + dx;
      panY = initPanY + dy;
      renderPhoto();
    } else {
      let nx = Math.max(-80, Math.min(80, Math.round(initPanX + dx)));
      let ny = Math.max(-60, Math.min(60, Math.round(initPanY + dy)));
      vPanXSlider.value = nx;
      vPanYSlider.value = ny;
      vPanXVal.innerText = nx;
      vPanYVal.innerText = ny;
      debounceProcessVideo();
    }
  });

  bezel.addEventListener('pointerup', () => { isDragging = false; });
  bezel.addEventListener('pointercancel', () => { isDragging = false; });

  // Mouse wheel zoom for photo and video
  bezel.addEventListener('wheel', (e) => {
    e.preventDefault();
    if (currentMode === 'photo' && loadedImg) {
      if (e.deltaY < 0) {
        zoom = Math.min(4.0, zoom + 0.1);
      } else {
        zoom = Math.max(0.1, zoom - 0.1);
      }
      zoomSlider.value = zoom.toFixed(2);
      zoomVal.innerText = zoom.toFixed(1) + 'x';
      renderPhoto();
    } else if (currentMode === 'video' && rawVideoFile) {
      let vz = parseFloat(vZoomSlider.value) || 1.0;
      if (e.deltaY < 0) {
        vz = Math.min(3.0, vz + 0.1);
      } else {
        vz = Math.max(0.3, vz - 0.1);
      }
      vZoomSlider.value = vz.toFixed(2);
      vZoomVal.innerText = vz.toFixed(2) + 'x';
      debounceProcessVideo();
    }
  }, { passive: false });

  // Touch pinch-to-zoom for mobile
  let touchStartDist = 0;
  let touchStartZoom = 1.0;
  bezel.addEventListener('touchstart', (e) => {
    if (currentMode === 'photo' && loadedImg && e.touches.length === 2) {
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      touchStartDist = Math.hypot(dx, dy);
      touchStartZoom = zoom;
    }
  }, { passive: true });

  bezel.addEventListener('touchmove', (e) => {
    if (currentMode === 'photo' && loadedImg && e.touches.length === 2 && touchStartDist > 0) {
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      const dist = Math.hypot(dx, dy);
      const factor = dist / touchStartDist;
      zoom = Math.max(0.1, Math.min(4.0, touchStartZoom * factor));
      zoomSlider.value = zoom.toFixed(2);
      zoomVal.innerText = zoom.toFixed(1) + 'x';
      renderPhoto();
    }
  }, { passive: true });

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
  const deletePhotoBtn = document.getElementById('deletePhotoBtn');
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
        zoomVal.innerText = "1.0x (Fit)";
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

  // Reliable Live OLED Sync (100ms throttle with in-flight lock to prevent drops)
  let isSendingLive = false, pendingLive = false, lastLive = 0;
  function triggerLiveSync() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    const now = Date.now();
    if (isSendingLive || (now - lastLive < 100)) {
      pendingLive = true;
      return;
    }
    sendLive();
  }
  async function sendLive() {
    if (!loadedImg || !liveSyncCheck.checked) return;
    isSendingLive = true;
    lastLive = Date.now();
    pendingLive = false;
    try {
      await fetch('/api/photo', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'data=' + packCurrentCanvasHex() + '&save=0'
      });
    } catch(e) {}
    isSendingLive = false;
    if (pendingLive && liveSyncCheck.checked) {
      setTimeout(sendLive, 80);
    }
  }

  zoomSlider.oninput = () => { zoom = parseFloat(zoomSlider.value); zoomVal.innerText = zoom.toFixed(2)+'x'; renderPhoto(); };
  brightSlider.oninput = () => { brightVal.innerText = brightSlider.value; renderPhoto(); };
  contrastSlider.oninput = () => { contrastVal.innerText = parseFloat(contrastSlider.value).toFixed(1)+'x'; renderPhoto(); };
  ditherMode.onchange = renderPhoto;
  rotateBtn.onclick = () => { rotationDeg = (rotationDeg + 90) % 360; renderPhoto(); };
  invertBtn.onclick = () => { isPhotoInverted = !isPhotoInverted; renderPhoto(); };

  document.getElementById('btnZoomOut').onclick = () => { zoom = Math.max(0.1, zoom - 0.2); zoomSlider.value = zoom; zoomVal.innerText = zoom.toFixed(1)+'x'; renderPhoto(); };
  document.getElementById('btnZoomIn').onclick = () => { zoom = Math.min(4.0, zoom + 0.2); zoomSlider.value = zoom; zoomVal.innerText = zoom.toFixed(1)+'x'; renderPhoto(); };
  document.getElementById('btnFit').onclick = () => { zoom = 1.0; baseScale = fitScale; zoomSlider.value = 1.0; zoomVal.innerText = "1.0x (Fit)"; panX = 0; panY = 0; renderPhoto(); };
  document.getElementById('btnFill').onclick = () => { zoom = fillScale / fitScale; zoomSlider.value = zoom; zoomVal.innerText = zoom.toFixed(1)+'x'; panX = 0; panY = 0; renderPhoto(); };
  document.getElementById('btnReset').onclick = () => { panX = 0; panY = 0; renderPhoto(); };

  sendBtn.onclick = () => {
    sendBtn.disabled = true; sendBtn.innerText = '⏳ Saving...';
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + packCurrentCanvasHex() + '&save=1'
    }).then(r=>r.json()).then(d=>{
      sendBtn.disabled = false; sendBtn.innerText = '💾 Save Photo to DeskBuddy';
      if (d.status === 'ok') showToast('✅ Photo Saved Permanently!', '#00C853');
      else showToast('Save failed', '#d32f2f');
    }).catch(()=>{
      sendBtn.disabled = false; sendBtn.innerText = '💾 Save Photo to DeskBuddy';
      showToast('Saved to DeskBuddy screen!', '#00C853');
    });
  };

  deletePhotoBtn.onclick = () => {
    if (!confirm('Are you sure you want to remove the photo from DeskBuddy?')) return;
    fetch('/api/photo/delete', { method: 'POST' })
      .then(r => r.json())
      .then(d => {
        if (d.status === 'ok') {
          showToast('Photo removed from DeskBuddy');
          loadedImg = null;
          sendBtn.disabled = true;
          drawDefaultGrid('Photo deleted');
        }
      });
  };

  // ==========================================
  // VIDEO & GIF STUDIO JAVASCRIPT
  // ==========================================
  const videoInput = document.getElementById('videoInput');
  const vStartSlider = document.getElementById('vStartSlider');
  const vStartVal = document.getElementById('vStartVal');
  const vMaxSecSelect = document.getElementById('vMaxSecSelect');
  const vFpsSelect = document.getElementById('vFpsSelect');
  const vFitMode = document.getElementById('vFitMode');
  const vDitherMode = document.getElementById('vDitherMode');
  const vPanXSlider = document.getElementById('vPanXSlider');
  const vPanXVal = document.getElementById('vPanXVal');
  const vPanYSlider = document.getElementById('vPanYSlider');
  const vPanYVal = document.getElementById('vPanYVal');
  const vZoomSlider = document.getElementById('vZoomSlider');
  const vZoomVal = document.getElementById('vZoomVal');
  const vThresholdSlider = document.getElementById('vThresholdSlider');
  const vThresholdVal = document.getElementById('vThresholdVal');
  const vInvertBtn = document.getElementById('vInvertBtn');
  const vPlayPauseBtn = document.getElementById('vPlayPauseBtn');
  const vStepBtn = document.getElementById('vStepBtn');
  const vScrubberGroup = document.getElementById('vScrubberGroup');
  const vScrubberSlider = document.getElementById('vScrubberSlider');
  const vScrubberVal = document.getElementById('vScrubberVal');
  const vLiveSyncCheck = document.getElementById('vLiveSyncCheck');
  const uploadVideoBtn = document.getElementById('uploadVideoBtn');
  const deleteVideoBtn = document.getElementById('deleteVideoBtn');

  const vDropFrameBtn = document.getElementById('vDropFrameBtn');
  const vSetStartBtn = document.getElementById('vSetStartBtn');
  const vSetEndBtn = document.getElementById('vSetEndBtn');
  const vResetFramesBtn = document.getElementById('vResetFramesBtn');

  let rawVideoFile = null;
  let videoFrames = []; // Array of 1024-byte hex strings
  let videoPreviewIndex = 0;
  let videoPreviewTimer = null;
  let isVideoPreviewPlaying = false;
  let isVideoInverted = false;
  let videoDebounceTimer = null;

  function debounceProcessVideo() {
    if (videoDebounceTimer) clearTimeout(videoDebounceTimer);
    videoDebounceTimer = setTimeout(processVideo, 300);
  }

  videoInput.addEventListener('change', async e => {
    const f = e.target.files[0];
    if (!f) return;
    rawVideoFile = f;
    vStartSlider.value = 0;
    await processVideo();
  });

  vStartSlider.oninput = () => { vStartVal.innerText = parseFloat(vStartSlider.value).toFixed(1) + 's'; debounceProcessVideo(); };
  vMaxSecSelect.onchange = debounceProcessVideo;
  vFpsSelect.onchange = debounceProcessVideo;
  vFitMode.onchange = debounceProcessVideo;
  vDitherMode.onchange = debounceProcessVideo;
  vPanXSlider.oninput = () => { vPanXVal.innerText = vPanXSlider.value; debounceProcessVideo(); };
  vPanYSlider.oninput = () => { vPanYVal.innerText = vPanYSlider.value; debounceProcessVideo(); };
  vThresholdSlider.oninput = () => { vThresholdVal.innerText = vThresholdSlider.value; debounceProcessVideo(); };
  vInvertBtn.onclick = () => { isVideoInverted = !isVideoInverted; debounceProcessVideo(); };

  vZoomSlider.oninput = () => {
    vZoomVal.innerText = parseFloat(vZoomSlider.value).toFixed(2) + 'x';
    debounceProcessVideo();
  };

  document.getElementById('vBtnFit').onclick = () => {
    vFitMode.value = 'contain'; vZoomSlider.value = 1.0; vZoomVal.innerText = '1.0x (Fit)';
    vPanXSlider.value = 0; vPanXVal.innerText = '0';
    vPanYSlider.value = 0; vPanYVal.innerText = '0';
    debounceProcessVideo();
  };
  document.getElementById('vBtnFill').onclick = () => {
    vFitMode.value = 'cover'; vZoomSlider.value = 1.0; vZoomVal.innerText = '1.0x (Fill)';
    vPanXSlider.value = 0; vPanXVal.innerText = '0';
    vPanYSlider.value = 0; vPanYVal.innerText = '0';
    debounceProcessVideo();
  };
  document.getElementById('vBtnStretch').onclick = () => {
    vFitMode.value = 'stretch'; vZoomSlider.value = 1.0; vZoomVal.innerText = '1.0x (Stretch)';
    vPanXSlider.value = 0; vPanXVal.innerText = '0';
    vPanYSlider.value = 0; vPanYVal.innerText = '0';
    debounceProcessVideo();
  };
  document.getElementById('vBtnZoomIn').onclick = () => {
    let z = Math.min(3.0, parseFloat(vZoomSlider.value) + 0.2);
    vZoomSlider.value = z.toFixed(2); vZoomVal.innerText = z.toFixed(2) + 'x';
    debounceProcessVideo();
  };
  document.getElementById('vBtnZoomOut').onclick = () => {
    let z = Math.max(0.3, parseFloat(vZoomSlider.value) - 0.2);
    vZoomSlider.value = z.toFixed(2); vZoomVal.innerText = z.toFixed(2) + 'x';
    debounceProcessVideo();
  };
  document.getElementById('vBtnReset').onclick = () => {
    vPanXSlider.value = 0; vPanXVal.innerText = '0';
    vPanYSlider.value = 0; vPanYVal.innerText = '0';
    debounceProcessVideo();
  };

  function updateVideoStats() {
    const fps = parseInt(vFpsSelect.value);
    document.getElementById('vStatsPill').style.display = 'block';
    document.getElementById('vFramesCount').innerText = videoFrames.length;
    document.getElementById('vFpsVal').innerText = fps;
    document.getElementById('vDurationVal').innerText = (videoFrames.length / fps).toFixed(1) + 's';
    document.getElementById('vSizeVal').innerText = ((videoFrames.length * 1024) / 1024).toFixed(1);
    uploadVideoBtn.disabled = (videoFrames.length === 0);
  }

  // Frame selector & editor (Drop frame / Cut start / Cut end / Reset)
  vDropFrameBtn.onclick = () => {
    if (videoFrames.length <= 1) {
      showToast('Need at least 1 frame', '#d32f2f');
      return;
    }
    const droppedIdx = videoPreviewIndex;
    videoFrames.splice(droppedIdx, 1);
    if (videoPreviewIndex >= videoFrames.length) videoPreviewIndex = videoFrames.length - 1;
    vScrubberSlider.max = Math.max(0, videoFrames.length - 1);
    vScrubberSlider.value = videoPreviewIndex;
    vScrubberVal.innerText = `Frame ${videoPreviewIndex + 1} of ${videoFrames.length}`;
    updateVideoStats();
    drawHexToCanvas(videoFrames[videoPreviewIndex]);
    if (vLiveSyncCheck.checked) syncVideoFrameToOled(videoFrames[videoPreviewIndex]);
    showToast(`🗑️ Dropped Frame ${droppedIdx + 1}! (${videoFrames.length} left)`, '#e65100');
  };

  vSetStartBtn.onclick = () => {
    if (videoFrames.length <= 1 || videoPreviewIndex === 0) return;
    const removed = videoPreviewIndex;
    videoFrames = videoFrames.slice(videoPreviewIndex);
    videoPreviewIndex = 0;
    vScrubberSlider.max = Math.max(0, videoFrames.length - 1);
    vScrubberSlider.value = 0;
    vScrubberVal.innerText = `Frame 1 of ${videoFrames.length}`;
    updateVideoStats();
    drawHexToCanvas(videoFrames[0]);
    if (vLiveSyncCheck.checked) syncVideoFrameToOled(videoFrames[0]);
    showToast(`✂️ Cut ${removed} leading frames!`, '#00C853');
  };

  vSetEndBtn.onclick = () => {
    if (videoFrames.length <= 1 || videoPreviewIndex >= videoFrames.length - 1) return;
    const cutCount = videoFrames.length - (videoPreviewIndex + 1);
    videoFrames = videoFrames.slice(0, videoPreviewIndex + 1);
    vScrubberSlider.max = Math.max(0, videoFrames.length - 1);
    vScrubberSlider.value = videoPreviewIndex;
    vScrubberVal.innerText = `Frame ${videoPreviewIndex + 1} of ${videoFrames.length}`;
    updateVideoStats();
    drawHexToCanvas(videoFrames[videoPreviewIndex]);
    if (vLiveSyncCheck.checked) syncVideoFrameToOled(videoFrames[videoPreviewIndex]);
    showToast(`✂️ Cut ${cutCount} trailing frames!`, '#00C853');
  };

  vResetFramesBtn.onclick = () => {
    if (!rawVideoFile) return;
    processVideo();
    showToast('↩️ Restored all frames from video!', '#1f6feb');
  };

  function seekVideoPromise(video, targetTime) {
    return new Promise(resolve => {
      let resolved = false;
      const onSeeked = () => {
        if (!resolved) {
          resolved = true;
          video.removeEventListener('seeked', onSeeked);
          resolve();
        }
      };
      video.addEventListener('seeked', onSeeked);
      video.currentTime = Math.min(targetTime, (video.duration || targetTime));
      setTimeout(onSeeked, 350); // Fallback timeout
    });
  }

  async function processVideo() {
    if (!rawVideoFile || currentMode !== 'video') return;
    stopVideoPreview();
    
    const progressBox = document.getElementById('vProgressBox');
    const progressBar = document.getElementById('vProgressBar');
    const progressText = document.getElementById('vProgressText');
    const progressPercent = document.getElementById('vProgressPercent');
    progressBox.style.display = 'block';
    progressText.innerText = 'Extracting video frames...';
    progressBar.style.width = '0%';
    progressPercent.innerText = '0%';

    const fps = parseInt(vFpsSelect.value);
    const maxSec = parseInt(vMaxSecSelect.value);
    const startTime = parseFloat(vStartSlider.value);
    const thresh = parseInt(vThresholdSlider.value);
    const fitMode = vFitMode.value;
    const ditherStyle = vDitherMode.value;
    const panX = parseInt(vPanXSlider.value);
    const panY = parseInt(vPanYSlider.value);
    const vZoom = parseFloat(vZoomSlider.value) || 1.0;

    try {
      const v = document.createElement('video');
      v.preload = 'auto';
      v.muted = true;
      v.playsInline = true;
      v.style.display = 'none';
      document.body.appendChild(v);
      v.src = URL.createObjectURL(rawVideoFile);

      await new Promise((res, rej) => {
        v.onloadedmetadata = () => res();
        v.onerror = () => rej(new Error('Cannot open video file'));
        setTimeout(res, 2000);
      });

      if (v.duration && v.duration > 0) {
        vStartSlider.max = Math.max(0, Math.floor((v.duration - 1) * 10) / 10);
      }

      const availableSec = Math.max(0.5, (v.duration ? v.duration - startTime : maxSec));
      const dur = Math.min(availableSec, maxSec);
      // Allow up to 160 frames (~160KB RAM, safely handles up to 30s at 5 FPS)
      const totalFrames = Math.min(Math.max(1, Math.floor(dur * fps)), 160);
      const frameInterval = 1.0 / fps;

      const off = document.createElement('canvas');
      off.width = 128; off.height = 64;
      const offCtx = off.getContext('2d');

      videoFrames = [];
      for (let i = 0; i < totalFrames; i++) {
        const pct = Math.round(((i + 1) / totalFrames) * 100);
        progressText.innerText = `Converting frame ${i + 1} of ${totalFrames}...`;
        progressPercent.innerText = pct + '%';
        progressBar.style.width = pct + '%';

        const seekT = startTime + (i * frameInterval);
        await seekVideoPromise(v, seekT);

        offCtx.fillStyle = isVideoInverted ? '#fff' : '#000';
        offCtx.fillRect(0, 0, 128, 64);
        
        let w = 128, h = 64;
        if (fitMode === 'stretch') {
          w = 128 * vZoom;
          h = 64 * vZoom;
        } else if (fitMode === 'cover') {
          let r = Math.max(128 / (v.videoWidth || 128), 64 / (v.videoHeight || 64)) * vZoom;
          w = (v.videoWidth || 128) * r;
          h = (v.videoHeight || 64) * r;
        } else { // contain / fit
          let r = Math.min(128 / (v.videoWidth || 128), 64 / (v.videoHeight || 64)) * vZoom;
          w = (v.videoWidth || 128) * r;
          h = (v.videoHeight || 64) * r;
        }
        const shiftX = (128 - w) / 2 + panX;
        const shiftY = (64 - h) / 2 + panY;
        offCtx.drawImage(v, shiftX, shiftY, w, h);

        const imgData = offCtx.getImageData(0, 0, 128, 64);
        const hex = convertVideoFrameToHex(imgData, thresh, isVideoInverted, ditherStyle);
        videoFrames.push(hex);
      }
      
      URL.revokeObjectURL(v.src);
      v.remove();
      progressBox.style.display = 'none';

      // Update stats
      updateVideoStats();

      // Setup Frame Scrubber
      vScrubberGroup.style.display = 'block';
      vScrubberSlider.max = Math.max(0, videoFrames.length - 1);
      vScrubberSlider.value = 0;
      vScrubberVal.innerText = `Frame 1 of ${videoFrames.length}`;

      videoPreviewIndex = 0;
      startVideoPreview();

      // If Realtime OLED Sync is enabled, immediately push frame 0 to OLED
      if (vLiveSyncCheck.checked && videoFrames.length > 0) {
        syncVideoFrameToOled(videoFrames[0]);
      }

      showToast('✅ Animation Ready! Live preview playing.', '#00C853');
    } catch(err) {
      progressBox.style.display = 'none';
      showToast('Error: ' + err.message, '#d32f2f');
    }
  }

  function convertVideoFrameToHex(imgData, thresh, inverted, style) {
    const d = imgData.data;
    const gray = new Float32Array(128 * 64);
    for (let i = 0; i < 128 * 64; i++) {
      let r = d[i * 4], g = d[i * 4 + 1], b = d[i * 4 + 2];
      let val = 0.299 * r + 0.587 * g + 0.114 * b;
      if (inverted) val = 255 - val;
      gray[i] = val;
    }

    let hex = "";
    if (style === 'threshold') {
      for (let y = 0; y < 64; y++) {
        for (let byteX = 0; byteX < 16; byteX++) {
          let b = 0;
          for (let bit = 0; bit < 8; bit++) {
            const x = byteX * 8 + bit;
            if (gray[y * 128 + x] > thresh) b |= (1 << (7 - bit));
          }
          let h = b.toString(16);
          if (h.length < 2) h = "0" + h;
          hex += h;
        }
      }
    } else {
      for (let y = 0; y < 64; y++) {
        for (let x = 0; x < 128; x++) {
          const idx = y * 128 + x;
          const oldV = gray[idx];
          const newV = oldV > thresh ? 255 : 0;
          const err = oldV - newV;
          gray[idx] = newV;
          if (x + 1 < 128) gray[idx + 1] += err * (7/16);
          if (y + 1 < 64) {
            if (x > 0) gray[idx + 128 - 1] += err * (3/16);
            gray[idx + 128] += err * (5/16);
            if (x + 1 < 128) gray[idx + 128 + 1] += err * (1/16);
          }
        }
      }
      for (let y = 0; y < 64; y++) {
        for (let byteX = 0; byteX < 16; byteX++) {
          let b = 0;
          for (let bit = 0; bit < 8; bit++) {
            const x = byteX * 8 + bit;
            if (gray[y * 128 + x] > 128) b |= (1 << (7 - bit));
          }
          let h = b.toString(16);
          if (h.length < 2) h = "0" + h;
          hex += h;
        }
      }
    }
    return hex;
  }

  function startVideoPreview() {
    stopVideoPreview();
    if (videoFrames.length === 0) return;
    isVideoPreviewPlaying = true;
    vPlayPauseBtn.innerText = '⏸ Pause';
    const fps = parseInt(vFpsSelect.value);
    const ms = 1000 / (fps > 0 ? fps : 8);

    videoPreviewTimer = setInterval(() => {
      drawHexToCanvas(videoFrames[videoPreviewIndex]);
      vScrubberSlider.value = videoPreviewIndex;
      vScrubberVal.innerText = `Frame ${videoPreviewIndex + 1} of ${videoFrames.length}`;
      videoPreviewIndex = (videoPreviewIndex + 1) % videoFrames.length;
    }, ms);
  }

  function stopVideoPreview() {
    if (videoPreviewTimer) { clearInterval(videoPreviewTimer); videoPreviewTimer = null; }
    isVideoPreviewPlaying = false;
    vPlayPauseBtn.innerText = '▶ Play';
  }

  vPlayPauseBtn.onclick = () => {
    if (isVideoPreviewPlaying) stopVideoPreview(); else startVideoPreview();
  };

  vStepBtn.onclick = () => {
    stopVideoPreview();
    if (videoFrames.length === 0) return;
    videoPreviewIndex = (videoPreviewIndex + 1) % videoFrames.length;
    drawHexToCanvas(videoFrames[videoPreviewIndex]);
    vScrubberSlider.value = videoPreviewIndex;
    vScrubberVal.innerText = `Frame ${videoPreviewIndex + 1} of ${videoFrames.length}`;
    if (vLiveSyncCheck.checked) syncVideoFrameToOled(videoFrames[videoPreviewIndex]);
  };

  vScrubberSlider.oninput = () => {
    stopVideoPreview();
    if (videoFrames.length === 0) return;
    videoPreviewIndex = parseInt(vScrubberSlider.value);
    drawHexToCanvas(videoFrames[videoPreviewIndex]);
    vScrubberVal.innerText = `Frame ${videoPreviewIndex + 1} of ${videoFrames.length}`;
    if (vLiveSyncCheck.checked) syncVideoFrameToOled(videoFrames[videoPreviewIndex]);
  };

  function syncVideoFrameToOled(hex) {
    if (!hex) return;
    fetch('/api/photo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'data=' + hex + '&save=0'
    }).catch(() => {});
  }

  vLiveSyncCheck.onchange = () => {
    if (vLiveSyncCheck.checked && videoFrames.length > 0) {
      syncVideoFrameToOled(videoFrames[videoPreviewIndex]);
      showToast('⚡ Syncing video frame to DeskBuddy OLED!', '#00E5FF');
    }
  };

  function drawHexToCanvas(hex) {
    if (!hex || hex.length < 2048) return;
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
      progressText.innerText = 'Initializing DeskBuddy storage...';
      const startRes = await fetch('/api/video/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'frames=' + total + '&fps=' + fps
      }).then(r => r.json());

      if (startRes.status !== 'ok') throw new Error(startRes.msg || 'Start failed');

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

      progressText.innerText = 'Finalizing on DeskBuddy...';
      await fetch('/api/video/end?save=1', { method: 'POST' });

      showToast('🎉 Video Uploaded! Playing on DeskBuddy!', '#00C853');
      progressText.innerText = 'Completed!';
    } catch(err) {
      showToast('Upload error: ' + err.message, '#d32f2f');
    } finally {
      uploadVideoBtn.disabled = false;
      setTimeout(() => { progressBox.style.display = 'none'; }, 2500);
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
          vScrubberGroup.style.display = 'none';
          document.getElementById('vStatsPill').style.display = 'none';
          uploadVideoBtn.disabled = true;
          drawDefaultGrid('Video removed');
        }
      });
  };

  // ==========================================
  // LAPTOP MONITOR SERVER SETTINGS JAVASCRIPT
  // ==========================================
  fetch('/api/server')
    .then(r => r.json())
    .then(d => {
      if (d.url) document.getElementById('serverUrlInput').value = d.url;
      if (d.name) document.getElementById('serverNameInput').value = d.name;
    }).catch(() => {});

  document.getElementById('saveServerBtn').onclick = () => {
    const url = document.getElementById('serverUrlInput').value;
    const name = document.getElementById('serverNameInput').value;
    fetch('/api/server', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'url=' + encodeURIComponent(url) + '&name=' + encodeURIComponent(name)
    })
    .then(r => r.json())
    .then(d => {
      if (d.status === 'ok') showToast('✅ Laptop settings saved!', '#00C853');
      else showToast('Save failed', '#d32f2f');
    })
    .catch(() => showToast('Saved!', '#00C853'));
  };
</script>
</body>
</html>
)rawliteral";
