#!/usr/bin/env python3
# ==============================================================================
# DeskBuddy Smart Brain Server (Mobile & Cloud AI Companion Backend)
# ==============================================================================
import os
import sys
import json
import time
import shutil
import asyncio
import tempfile
import subprocess
from pathlib import Path
from typing import Optional, List

from fastapi import FastAPI, UploadFile, File, Form, Request, BackgroundTasks
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import requests
from PIL import Image, ImageDraw, ImageFont, ImageOps

CONFIG_FILE = Path(__file__).parent / "config.json"

DEFAULT_CONFIG = {
    "deskbuddy_ip": "10.91.8.125",
    "gemini_api_key": "",
    "model_name": "gemini-1.5-flash",
    "default_fps": 10,
    "default_dither": "floyd",
    "voice": "hi-IN-SwaraNeural"
}

def load_config():
    if not CONFIG_FILE.exists():
        save_config(DEFAULT_CONFIG)
        return DEFAULT_CONFIG.copy()
    try:
        with open(CONFIG_FILE, "r", encoding="utf-8") as f:
            data = json.load(f)
            merged = DEFAULT_CONFIG.copy()
            merged.update(data)
            return merged
    except Exception as e:
        print(f"[CONFIG] Error loading config: {e}")
        return DEFAULT_CONFIG.copy()

def save_config(cfg):
    try:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump(cfg, f, indent=2)
    except Exception as e:
        print(f"[CONFIG] Error saving config: {e}")

cfg = load_config()

app = FastAPI(title="DeskBuddy Brain Server")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ==============================================================================
# Helper: Pillow Image to SSD1306 1024-Byte Hex String
# ==============================================================================
def pil_to_ssd1306_hex(img: Image.Image, dither_mode: str = "floyd", invert: bool = False) -> str:
    # 1. Resize & Crop to 128x64 maintaining aspect ratio
    img = img.convert("RGB")
    target_w, target_h = 128, 64
    img_ratio = img.width / img.height
    target_ratio = target_w / target_h

    if img_ratio > target_ratio:
        new_w = int(target_h * img_ratio)
        img = img.resize((new_w, target_h), Image.Resampling.LANCZOS)
        left = (new_w - target_w) // 2
        img = img.crop((left, 0, left + target_w, target_h))
    else:
        new_h = int(target_w / img_ratio)
        img = img.resize((target_w, new_h), Image.Resampling.LANCZOS)
        top = (new_h - target_h) // 2
        img = img.crop((0, top, target_w, top + target_h))

    # 2. Dither / Binarize
    if dither_mode == "floyd":
        img_1bit = img.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
    else:
        # Crisp threshold
        gray = img.convert("L")
        img_1bit = gray.point(lambda p: 255 if p > 128 else 0, mode='1')

    if invert:
        img_1bit = ImageOps.invert(img_1bit.convert("L")).convert("1")

    # 3. Pack 128x64 to 1024 bytes (16 bytes per row, MSB first)
    pixels = img_1bit.load()
    hex_parts = []
    for y in range(64):
        for byte_x in range(16):
            b = 0
            for bit in range(8):
                x = byte_x * 8 + bit
                # in mode 1, 255 is White (LED ON)
                if pixels[x, y] > 128:
                    b |= (1 << (7 - bit))
            hex_parts.append(f"{b:02x}")

    return "".join(hex_parts)

# ==============================================================================
# Helper: Push Hex Frame / Photo to DeskBuddy ESP32
# ==============================================================================
def send_photo_to_deskbuddy(hex_str: str, save_permanent: bool = True) -> bool:
    target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
    url = f"http://{target_ip}/api/photo"
    try:
        r = requests.post(
            url,
            data={"data": hex_str, "save": "1" if save_permanent else "0"},
            timeout=3.0
        )
        return r.status_code == 200 and r.json().get("status") == "ok"
    except Exception as e:
        print(f"[DESKBUDDY] Photo send failed: {e}")
        return False

# ==============================================================================
# Helper: Render AI Text / Message directly onto 128x64 OLED Bitmap
# ==============================================================================
def render_text_to_oled_hex(text: str, emotion: str = "HAPPY", title: str = "DESKBUDDY AI") -> str:
    img = Image.new("1", (128, 64), 0)
    draw = ImageDraw.Draw(img)

    # Header bar
    draw.line([(0, 11), (127, 11)], fill=1)
    # Header Title
    draw.text((2, 1), f"🤖 {title[:14]}", fill=1)
    # Emotion tag in corner
    draw.text((88, 1), f"[{emotion[:4]}]", fill=1)

    # Word wrap text for 128x64 (approx 20 chars per line, 4 lines)
    words = text.split()
    lines = []
    cur_line = ""
    for w in words:
        test = (cur_line + " " + w).strip()
        if len(test) <= 21:
            cur_line = test
        else:
            lines.append(cur_line)
            cur_line = w
            if len(lines) >= 4:
                break
    if cur_line and len(lines) < 4:
        lines.append(cur_line)

    y = 15
    for line in lines:
        draw.text((2, y), line, fill=1)
        y += 12

    # Return packed hex string
    pixels = img.load()
    hex_parts = []
    for row_y in range(64):
        for byte_x in range(16):
            b = 0
            for bit in range(8):
                x = byte_x * 8 + bit
                if pixels[x, row_y] > 0:
                    b |= (1 << (7 - bit))
            hex_parts.append(f"{b:02x}")
    return "".join(hex_parts)

# ==============================================================================
# AI Brain: Google Gemini 1.5 Flash
# ==============================================================================
SYSTEM_PROMPT = """You are DeskBuddy, an adorable, witty, curious desktop robot pet companion!
Rules for your answers:
1. Answer in 1 to 2 short, punchy, conversational sentences (suitable for a tiny 128x64 OLED display).
2. You naturally understand and respond in English or Hinglish/Hindi based on user's query.
3. Always prefix your answer with ONE emotion tag from:
   [HAPPY], [EXCITED], [THINKING], [LOVE], [CHILL], [WINK], [ANNOYED], [SURPRISED], [QUESTION]
Example:
User: How are you?
Response: [HAPPY] I'm buzzing with energy! Ready to make your desk super productive today!
"""

def query_gemini_ai(user_prompt: str) -> tuple[str, str]:
    api_key = cfg.get("gemini_api_key", "").strip()
    if not api_key:
        return "[ANNOYED]", "Please set your Gemini API key in Settings tab first!"

    url = f"https://generativelanguage.googleapis.com/v1beta/models/{cfg.get('model_name', 'gemini-1.5-flash')}:generateContent?key={api_key}"
    payload = {
        "contents": [
            {
                "role": "user",
                "parts": [{"text": f"{SYSTEM_PROMPT}\n\nUser: {user_prompt}"}]
            }
        ],
        "generationConfig": {
            "temperature": 0.7,
            "maxOutputTokens": 100,
        }
    }

    try:
        r = requests.post(url, json=payload, timeout=8.0)
        if r.status_code != 200:
            return "[ANNOYED]", f"AI Error: {r.status_code} ({r.text[:60]})"
        res_json = r.json()
        raw_text = res_json["candidates"][0]["content"]["parts"][0]["text"].strip()

        # Parse emotion tag
        emotion = "HAPPY"
        clean_text = raw_text
        if raw_text.startswith("[") and "]" in raw_text:
            end_idx = raw_text.find("]")
            emotion = raw_text[1:end_idx].strip().upper()
            clean_text = raw_text[end_idx+1:].strip()

        return emotion, clean_text
    except Exception as e:
        return "[ANNOYED]", f"Connection error: {str(e)[:60]}"

# ==============================================================================
# API Endpoints
# ==============================================================================
@app.get("/api/config")
def get_config_endpoint():
    return cfg

@app.post("/api/config")
def update_config_endpoint(data: dict):
    global cfg
    for k in ["deskbuddy_ip", "gemini_api_key", "default_fps", "default_dither"]:
        if k in data:
            cfg[k] = data[k]
    save_config(cfg)
    return {"status": "ok", "config": cfg}

@app.get("/api/ping")
def ping_deskbuddy():
    target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
    try:
        t0 = time.time()
        r = requests.get(f"http://{target_ip}/api/photo", timeout=2.0)
        latency_ms = int((time.time() - t0) * 1000)
        return {"online": True, "ip": target_ip, "latency_ms": latency_ms}
    except Exception:
        return {"online": False, "ip": target_ip, "latency_ms": -1}

class AIChatRequest(BaseModel):
    message: str
    push_to_oled: bool = True

@app.post("/api/ai/chat")
def chat_ai_endpoint(req: AIChatRequest):
    emotion, reply = query_gemini_ai(req.message)
    pushed = False
    if req.push_to_oled:
        hex_data = render_text_to_oled_hex(reply, emotion=emotion, title="BUDDY AI")
        pushed = send_photo_to_deskbuddy(hex_data, save_permanent=False)
    return {
        "status": "ok",
        "emotion": emotion,
        "reply": reply,
        "pushed_to_oled": pushed
    }

@app.post("/api/media/upload-photo")
async def upload_photo(
    file: UploadFile = File(...),
    dither: str = Form("floyd"),
    invert: bool = Form(False),
    save: bool = Form(True)
):
    try:
        contents = await file.read()
        with tempfile.NamedTemporaryFile(delete=False, suffix=".img") as tmp:
            tmp.write(contents)
            tmp_path = tmp.name

        img = Image.open(tmp_path)
        hex_str = pil_to_ssd1306_hex(img, dither_mode=dither, invert=invert)
        os.unlink(tmp_path)

        ok = send_photo_to_deskbuddy(hex_str, save_permanent=save)
        return {"status": "ok" if ok else "failed", "pushed": ok}
    except Exception as e:
        return {"status": "error", "message": str(e)}

@app.post("/api/media/upload-video")
async def upload_video(
    file: UploadFile = File(...),
    fps: int = Form(10),
    max_duration: int = Form(8),
    dither: str = Form("floyd"),
    invert: bool = Form(False)
):
    """Instant Video / GIF Converter using FFmpeg & In-Memory Pipeline"""
    if fps <= 0 or fps > 20: fps = 10
    if max_duration <= 0 or max_duration > 10: max_duration = 8
    target_frames = min(100, fps * max_duration)

    contents = await file.read()
    suffix = Path(file.filename).suffix or ".mp4"
    with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as tmp_in:
        tmp_in.write(contents)
        input_file = tmp_in.name

    out_dir = tempfile.mkdtemp()
    try:
        # Check if ffmpeg is installed
        ffmpeg_bin = shutil.which("ffmpeg")
        if not ffmpeg_bin:
            return JSONResponse(
                status_code=500,
                content={"status": "error", "message": "FFmpeg not found! Run 'pkg install ffmpeg' in Termux."}
            )

        # Fast extract frames directly resized to 128x64 using FFmpeg
        out_pattern = os.path.join(out_dir, "frame_%04d.png")
        cmd = [
            ffmpeg_bin, "-y", "-i", input_file,
            "-t", str(max_duration),
            "-vf", f"fps={fps},scale=128:64:force_original_aspect_ratio=increase,crop=128:64",
            out_pattern
        ]
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.returncode != 0:
            return JSONResponse(status_code=500, content={"status": "error", "message": "FFmpeg conversion failed"})

        frame_files = sorted(Path(out_dir).glob("frame_*.png"))[:target_frames]
        if not frame_files:
            return JSONResponse(status_code=400, content={"status": "error", "message": "No frames extracted"})

        # Convert each frame to 1024-byte hex string in RAM
        hex_frames = []
        for fp in frame_files:
            with Image.open(fp) as f_img:
                hex_frames.append(pil_to_ssd1306_hex(f_img, dither_mode=dither, invert=invert))

        # Push to DeskBuddy ESP32
        target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
        start_res = requests.post(
            f"http://{target_ip}/api/video/start",
            data={"frames": len(hex_frames), "fps": fps},
            timeout=3.0
        ).json()
        if start_res.get("status") != "ok":
            return {"status": "error", "message": "DeskBuddy rejected video start"}

        for idx, h_data in enumerate(hex_frames):
            requests.post(
                f"http://{target_ip}/api/video/frame",
                data={"idx": idx, "data": h_data},
                timeout=2.0
            )

        requests.post(f"http://{target_ip}/api/video/end?save=1", timeout=3.0)
        return {
            "status": "ok",
            "frames_uploaded": len(hex_frames),
            "fps": fps,
            "duration": round(len(hex_frames) / fps, 1)
        }
    except Exception as e:
        return {"status": "error", "message": str(e)}
    finally:
        if os.path.exists(input_file): os.unlink(input_file)
        shutil.rmtree(out_dir, ignore_errors=True)

# ==============================================================================
# Web UI Dashboard (Mobile First, Beautiful Modern Dark Theme)
# ==============================================================================
@app.get("/", response_class=HTMLResponse)
def index_page():
    return """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>DeskBuddy Brain Server</title>
<style>
  :root {
    --bg: #0b0e14;
    --card: #151922;
    --border: #262c38;
    --accent: #00E676;
    --accent-blue: #00E5FF;
    --text: #e1e7ec;
    --text-muted: #8b949e;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: var(--bg);
    color: var(--text);
    padding: 14px;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
  }
  .container { width: 100%; max-width: 480px; }
  header { text-align: center; margin-bottom: 16px; }
  h1 { font-size: 22px; color: var(--accent); display: flex; align-items: center; justify-content: center; gap: 8px; }
  .status-badge {
    display: inline-flex; align-items: center; gap: 6px;
    background: rgba(0,230,118,0.1); border: 1px solid var(--accent);
    color: var(--accent); font-size: 11px; padding: 4px 10px; border-radius: 12px; margin-top: 6px;
  }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--accent); animation: pulse 2s infinite; }
  @keyframes pulse { 0% { opacity: 0.4; } 50% { opacity: 1; } 100% { opacity: 0.4; } }

  .tabs { display: flex; gap: 6px; margin-bottom: 14px; }
  .tab-btn {
    flex: 1; padding: 10px 4px; border: 1px solid var(--border); background: var(--card);
    color: var(--text-muted); border-radius: 8px; font-size: 13px; font-weight: 600; cursor: pointer; text-align: center;
  }
  .tab-btn.active { background: rgba(0,230,118,0.15); border-color: var(--accent); color: var(--accent); }
  .tab-pane { display: none; }
  .tab-pane.active { display: block; }

  .card {
    background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 16px; margin-bottom: 14px;
  }
  h2 { font-size: 15px; margin-bottom: 12px; color: var(--accent-blue); display: flex; align-items: center; gap: 6px; }

  input, select, textarea {
    width: 100%; padding: 10px 12px; background: #0e121a; border: 1px solid var(--border);
    border-radius: 8px; color: #fff; font-size: 14px; margin-bottom: 10px;
  }
  input:focus, textarea:focus { border-color: var(--accent); outline: none; }

  .btn {
    width: 100%; padding: 12px; background: var(--accent); color: #000; font-weight: bold;
    font-size: 14px; border: none; border-radius: 8px; cursor: pointer; transition: 0.2s;
  }
  .btn:active { transform: scale(0.98); }
  .btn:disabled { opacity: 0.5; cursor: not-allowed; }
  .btn-blue { background: var(--accent-blue); color: #000; }
  .btn-outline { background: transparent; border: 1px solid var(--border); color: var(--text); }

  .chat-box {
    height: 240px; overflow-y: auto; background: #0e121a; border: 1px solid var(--border);
    border-radius: 8px; padding: 10px; margin-bottom: 10px; display: flex; flex-direction: column; gap: 8px;
  }
  .msg { padding: 8px 12px; border-radius: 10px; font-size: 13px; max-width: 85%; }
  .msg-user { background: #1e2638; align-self: flex-end; border-bottom-right-radius: 2px; }
  .msg-bot { background: rgba(0,230,118,0.15); border: 1px solid var(--accent); align-self: flex-start; border-bottom-left-radius: 2px; }

  .toast {
    position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%);
    background: #00C853; color: #000; padding: 10px 20px; border-radius: 20px;
    font-weight: bold; font-size: 13px; display: none; z-index: 1000;
  }
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>🤖 DeskBuddy Brain</h1>
    <div class="status-badge" id="statusBadge">
      <div class="dot" id="statusDot"></div>
      <span id="statusText">Checking DeskBuddy...</span>
    </div>
  </header>

  <div class="tabs">
    <div class="tab-btn active" onclick="switchTab('chat')">💬 AI Chat</div>
    <div class="tab-btn" onclick="switchTab('media')">🎬 Media</div>
    <div class="tab-btn" onclick="switchTab('settings')">⚙️ Settings</div>
  </div>

  <!-- TAB: AI CHAT -->
  <div id="tab-chat" class="tab-pane active">
    <div class="card">
      <h2>🤖 Chat with DeskBuddy AI (Gemini 1.5)</h2>
      <div class="chat-box" id="chatBox">
        <div class="msg msg-bot"><b>DeskBuddy:</b> Hi! I'm your AI buddy. Ask me anything, and I'll display the answer on my screen!</div>
      </div>
      <input type="text" id="chatInput" placeholder="Ask a question or say hi..." onkeydown="if(event.key==='Enter') sendChat()">
      <button class="btn" id="sendChatBtn" onclick="sendChat()">⚡ Ask & Show on DeskBuddy</button>
    </div>
  </div>

  <!-- TAB: MEDIA CONVERTER -->
  <div id="tab-media" class="tab-pane">
    <div class="card">
      <h2>🎬 Instant Video / GIF Upload</h2>
      <p style="font-size:12px; color:var(--text-muted); margin-bottom:10px;">Phone converts video in milliseconds and flashes directly to DeskBuddy!</p>
      <input type="file" id="videoFile" accept="video/*,image/gif">
      <div style="display:flex; gap:8px;">
        <select id="videoFps" style="flex:1;">
          <option value="8">8 FPS (Smooth)</option>
          <option value="10" selected>10 FPS (Best Balance)</option>
          <option value="12">12 FPS (Fluid)</option>
        </select>
        <select id="videoDuration" style="flex:1;">
          <option value="4">4s Max</option>
          <option value="6">6s Max</option>
          <option value="8" selected>8s Max (Recommended)</option>
          <option value="10">10s Max</option>
        </select>
      </div>
      <button class="btn btn-blue" id="uploadVideoBtn" onclick="uploadVideo()">⚡ Convert & Push Video</button>
    </div>

    <div class="card">
      <h2>📸 Instant Photo Upload</h2>
      <input type="file" id="photoFile" accept="image/*">
      <button class="btn" id="uploadPhotoBtn" onclick="uploadPhoto()">⚡ Push Photo to Screen</button>
    </div>
  </div>

  <!-- TAB: SETTINGS -->
  <div id="tab-settings" class="tab-pane">
    <div class="card">
      <h2>⚙️ System Configuration</h2>
      <label style="font-size:12px; color:var(--text-muted);">DeskBuddy IP Address:</label>
      <input type="text" id="cfgIp" placeholder="e.g. 10.91.8.125">

      <label style="font-size:12px; color:var(--text-muted);">Google Gemini API Key:</label>
      <input type="password" id="cfgKey" placeholder="AIzaSy...">
      <p style="font-size:11px; color:var(--text-muted); margin-bottom:12px;">Get a 100% free key at <a href="https://aistudio.google.com/app/apikey" target="_blank" style="color:var(--accent);">aistudio.google.com</a> (No credit card required)</p>

      <button class="btn" onclick="saveSettings()">💾 Save Settings</button>
    </div>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
  function switchTab(name) {
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
    event.target.classList.add('active');
    document.getElementById('tab-' + name).classList.add('active');
  }

  function showToast(text, color='#00C853') {
    const t = document.getElementById('toast');
    t.innerText = text;
    t.style.background = color;
    t.style.display = 'block';
    setTimeout(() => { t.style.display = 'none'; }, 3000);
  }

  async function checkPing() {
    try {
      const r = await fetch('/api/ping');
      const d = await r.json();
      const dot = document.getElementById('statusDot');
      const txt = document.getElementById('statusText');
      if (d.online) {
        dot.style.background = '#00E676';
        txt.innerText = `DeskBuddy Online (${d.latency_ms}ms)`;
      } else {
        dot.style.background = '#FF5252';
        txt.innerText = 'DeskBuddy Offline (' + d.ip + ')';
      }
    } catch(e) {}
  }
  setInterval(checkPing, 5000);
  checkPing();

  async function loadSettings() {
    const r = await fetch('/api/config');
    const d = await r.json();
    document.getElementById('cfgIp').value = d.deskbuddy_ip || '';
    document.getElementById('cfgKey').value = d.gemini_api_key || '';
  }
  loadSettings();

  async function saveSettings() {
    const ip = document.getElementById('cfgIp').value.trim();
    const key = document.getElementById('cfgKey').value.trim();
    const r = await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ deskbuddy_ip: ip, gemini_api_key: key })
    });
    if (r.ok) {
      showToast('Settings saved!');
      checkPing();
    }
  }

  async function sendChat() {
    const inp = document.getElementById('chatInput');
    const text = inp.value.trim();
    if (!text) return;
    inp.value = '';

    const box = document.getElementById('chatBox');
    box.innerHTML += `<div class="msg msg-user"><b>You:</b> ${text}</div>`;
    box.scrollTop = box.scrollHeight;

    const btn = document.getElementById('sendChatBtn');
    btn.disabled = true; btn.innerText = '🤖 Thinking...';

    try {
      const r = await fetch('/api/ai/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: text, push_to_oled: true })
      });
      const d = await r.json();
      btn.disabled = false; btn.innerText = '⚡ Ask & Show on DeskBuddy';
      box.innerHTML += `<div class="msg msg-bot"><b>${d.emotion}:</b> ${d.reply}</div>`;
      box.scrollTop = box.scrollHeight;
      if (d.pushed_to_oled) {
        showToast('Sent to DeskBuddy OLED!', '#00C853');
      }
    } catch(err) {
      btn.disabled = false; btn.innerText = '⚡ Ask & Show on DeskBuddy';
      box.innerHTML += `<div class="msg msg-bot" style="color:#ff5252;">Error: ${err.message}</div>`;
    }
  }

  async function uploadPhoto() {
    const f = document.getElementById('photoFile').files[0];
    if (!f) return alert('Please select a photo');
    const btn = document.getElementById('uploadPhotoBtn');
    btn.disabled = true; btn.innerText = 'Processing...';

    const fd = new FormData();
    fd.append('file', f);
    try {
      const r = await fetch('/api/media/upload-photo', { method: 'POST', body: fd });
      const d = await r.json();
      btn.disabled = false; btn.innerText = '⚡ Push Photo to Screen';
      if (d.pushed) showToast('Photo uploaded to DeskBuddy!', '#00C853');
      else showToast('Upload failed', '#d32f2f');
    } catch(e) {
      btn.disabled = false; btn.innerText = '⚡ Push Photo to Screen';
      alert('Error: ' + e.message);
    }
  }

  async function uploadVideo() {
    const f = document.getElementById('videoFile').files[0];
    if (!f) return alert('Please select a video file');
    const btn = document.getElementById('uploadVideoBtn');
    btn.disabled = true; btn.innerText = 'Converting in RAM & Flashing...';

    const fd = new FormData();
    fd.append('file', f);
    fd.append('fps', document.getElementById('videoFps').value);
    fd.append('max_duration', document.getElementById('videoDuration').value);

    try {
      const r = await fetch('/api/media/upload-video', { method: 'POST', body: fd });
      const d = await r.json();
      btn.disabled = false; btn.innerText = '⚡ Convert & Push Video';
      if (d.status === 'ok') {
        showToast(`🎉 ${d.frames_uploaded} Frames (${d.duration}s) Playing on DeskBuddy!`, '#00C853');
      } else {
        showToast('Error: ' + d.message, '#d32f2f');
      }
    } catch(e) {
      btn.disabled = false; btn.innerText = '⚡ Convert & Push Video';
      alert('Error: ' + e.message);
    }
  }
</script>
</body>
</html>
"""

if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 8000))
    print(f"🚀 DeskBuddy Brain Server starting on port {port}...")
    uvicorn.run(app, host="0.0.0.0", port=port)
