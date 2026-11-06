#!/usr/bin/env python3
# ==============================================================================
# DeskBuddy Smart Brain Server (Android Termux & Cloud AI Companion Backend)
# Powered by Flask (100% Pure Python - Zero Rust / Zero Maturin build needed!)
# Supports Google Gemini (AIzaSy...) & Groq Cloud (gsk_...)
# ==============================================================================
import os
import sys
import json
import time
import shutil
import socket
import tempfile
import subprocess
import re
import threading
import base64
import urllib.parse
from pathlib import Path

from flask import Flask, request, jsonify, render_template_string, send_from_directory
import requests
from PIL import Image, ImageDraw, ImageOps

CONFIG_FILE = Path(__file__).parent / "config.json"
AUDIO_DIR = Path(__file__).parent / "static" / "audio"
AUDIO_DIR.mkdir(parents=True, exist_ok=True)

DEFAULT_CONFIG = {
    "deskbuddy_ip": "10.91.8.125",
    "gemini_api_key": "",
    "model_name": "gemini-3.5-flash-lite",
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

def save_config(c):
    try:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump(c, f, indent=2)
    except Exception as e:
        print(f"[CONFIG] Error saving config: {e}")

cfg = load_config()

app = Flask(__name__)

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

# ==============================================================================
# Cloudflare Global Tunnel Manager (Anywhere in the world access)
# ==============================================================================
tunnel_process = None
tunnel_url = None
tunnel_lock = threading.Lock()

def ensure_cloudflared_installed():
    if shutil.which("cloudflared"):
        return True
    # Try pkg install in Termux
    try:
        subprocess.run(["pkg", "install", "-y", "cloudflared"], capture_output=True, timeout=40)
        if shutil.which("cloudflared"):
            return True
    except Exception:
        pass
    # Try downloading official static ARM binary
    try:
        import platform
        arch = platform.machine()
        bin_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64" if ("aarch" in arch or "arm64" in arch) else "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm"
        target_path = os.environ.get("PREFIX", "/data/data/com.termux/files/usr") + "/bin/cloudflared"
        r = requests.get(bin_url, timeout=30, stream=True)
        if r.status_code == 200:
            with open(target_path, "wb") as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
            os.chmod(target_path, 0o755)
            return True
    except Exception as e:
        print(f"[TUNNEL] Error fetching cloudflared: {e}")
    return False

def start_tunnel_thread(local_port=8000):
    global tunnel_process, tunnel_url
    with tunnel_lock:
        if tunnel_process and tunnel_process.poll() is None and tunnel_url:
            return tunnel_url

        if not shutil.which("cloudflared"):
            print("[TUNNEL] Checking cloudflared binary...")
            ensure_cloudflared_installed()

        if not shutil.which("cloudflared"):
            print("[TUNNEL] Cloudflared not found. Install via: pkg install cloudflared")
            return None

        try:
            cmd = ["cloudflared", "tunnel", "--url", f"http://127.0.0.1:{local_port}"]
            tunnel_process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
        except Exception as e:
            print(f"[TUNNEL] Failed to start cloudflared: {e}")
            return None

    def reader():
        global tunnel_url
        # Exclude api.trycloudflare.com - that's just Cloudflare's API, not the tunnel URL
        url_pattern = re.compile(r"https://(?!api\.)[a-zA-Z0-9-]+\.trycloudflare\.com")
        for line in iter(tunnel_process.stdout.readline, ''):
            if not line:
                break
            line_stripped = line.strip()
            # Print all cloudflared output so URL is always visible in terminal
            if line_stripped:
                print(f"[cloudflared] {line_stripped}")
            m = url_pattern.search(line)
            if m and not tunnel_url:
                tunnel_url = m.group(0)
                print("\n" + "=" * 65)
                print("🌐 CLOUDFLARE GLOBAL URL (ACCESS FROM ANYWHERE IN THE WORLD):")
                print(f"👉 {tunnel_url}")
                print("=" * 65 + "\n")
        if tunnel_process and tunnel_process.stdout:
            tunnel_process.stdout.close()

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    # Wait up to 12s for tunnel URL
    t0 = time.time()
    while time.time() - t0 < 12:
        if tunnel_url:
            return tunnel_url
        time.sleep(0.3)
    return tunnel_url

def stop_tunnel():
    global tunnel_process, tunnel_url
    with tunnel_lock:
        if tunnel_process:
            try:
                tunnel_process.terminate()
                tunnel_process.wait(timeout=3)
            except Exception:
                try:
                    tunnel_process.kill()
                except Exception:
                    pass
            tunnel_process = None
        tunnel_url = None

# ==============================================================================
# Helper: Pillow Image to SSD1306 1024-Byte Hex String
# ==============================================================================
def pil_to_ssd1306_hex(img: Image.Image, dither_mode: str = "floyd", invert: bool = False) -> str:
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

    if dither_mode == "floyd":
        img_1bit = img.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
    else:
        gray = img.convert("L")
        img_1bit = gray.point(lambda p: 255 if p > 128 else 0, mode='1')

    if invert:
        img_1bit = ImageOps.invert(img_1bit.convert("L")).convert("1")

    pixels = img_1bit.load()
    hex_parts = []
    for y in range(64):
        for byte_x in range(16):
            b = 0
            for bit in range(8):
                x = byte_x * 8 + bit
                if pixels[x, y] > 128:
                    b |= (1 << (7 - bit))
            hex_parts.append(f"{b:02x}")

    return "".join(hex_parts)

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

def render_text_to_oled_hex(text: str, emotion: str = "HAPPY", title: str = "BUDDY AI") -> str:
    img = Image.new("1", (128, 64), 0)
    draw = ImageDraw.Draw(img)

    draw.line([(0, 11), (127, 11)], fill=1)
    draw.text((2, 1), f"🤖 {title[:12]}", fill=1)
    draw.text((86, 1), f"[{emotion[:4]}]", fill=1)

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
# AI Brain: Supports Google Gemini & Groq Cloud
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

def query_ai(user_prompt: str) -> tuple[str, str]:
    api_key = cfg.get("gemini_api_key", "").strip()
    if not api_key:
        return "[ANNOYED]", "Please enter your API Key in Settings tab first!"

    # 1. Support Groq Cloud (Ultra-Fast Fallback if key starts with gsk_)
    if api_key.startswith("gsk_"):
        try:
            url = "https://api.groq.com/openai/v1/chat/completions"
            headers = {"Authorization": f"Bearer {api_key}", "Content-Type": "application/json"}
            payload = {
                "model": "llama-3.1-8b-instant",
                "messages": [
                    {"role": "system", "content": SYSTEM_PROMPT},
                    {"role": "user", "content": user_prompt}
                ],
                "max_tokens": 80,
                "temperature": 0.7
            }
            r = requests.post(url, headers=headers, json=payload, timeout=6.0)
            if r.status_code != 200:
                return "[ANNOYED]", f"Groq Error {r.status_code}: {r.text[:50]}"
            raw_text = r.json()["choices"][0]["message"]["content"].strip()
            emotion = "HAPPY"
            clean_text = raw_text
            if raw_text.startswith("[") and "]" in raw_text:
                end_idx = raw_text.find("]")
                emotion = raw_text[1:end_idx].strip().upper()
                clean_text = raw_text[end_idx+1:].strip()
            return emotion, clean_text
        except Exception as e:
            return "[ANNOYED]", f"Groq Error: {str(e)[:50]}"

    # 2. Google Gemini API (Auto-fallback to active flash models)
    models_to_try = [
        cfg.get("model_name", "gemini-3.5-flash-lite"),
        "gemini-3.5-flash",
        "gemini-3.1-flash-lite",
        "gemini-3.7-flash",
        "gemini-flash-latest",
        "gemini-1.5-flash"
    ]
    last_err = ""
    for m in models_to_try:
        url = f"https://generativelanguage.googleapis.com/v1beta/models/{m}:generateContent?key={api_key}"
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
            if r.status_code == 200:
                res_json = r.json()
                raw_text = res_json["candidates"][0]["content"]["parts"][0]["text"].strip()
                emotion = "HAPPY"
                clean_text = raw_text
                if raw_text.startswith("[") and "]" in raw_text:
                    end_idx = raw_text.find("]")
                    emotion = raw_text[1:end_idx].strip().upper()
                    clean_text = raw_text[end_idx+1:].strip()
                return emotion, clean_text
            else:
                try:
                    err_msg = r.json().get("error", {}).get("message", r.text[:60])
                except Exception:
                    err_msg = r.text[:60]
                last_err = f"API Error {r.status_code}: {err_msg}"
        except Exception as e:
            last_err = f"Connection error: {str(e)[:50]}"

    return "[ANNOYED]", last_err

# ==============================================================================
# Routes
# ==============================================================================
@app.route("/api/config", methods=["GET", "POST"])
def config_route():
    global cfg
    if request.method == "POST":
        data = request.get_json(silent=True) or {}
        for k in ["deskbuddy_ip", "gemini_api_key", "default_fps", "default_dither", "voice"]:
            if k in data:
                cfg[k] = data[k]
        save_config(cfg)
        return jsonify({"status": "ok", "config": cfg})
    return jsonify(cfg)

@app.route("/api/ping", methods=["GET"])
def ping_route():
    target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
    try:
        t0 = time.time()
        requests.get(f"http://{target_ip}/api/photo", timeout=2.0)
        latency_ms = int((time.time() - t0) * 1000)
        return jsonify({"online": True, "ip": target_ip, "latency_ms": latency_ms})
    except Exception:
        return jsonify({"online": False, "ip": target_ip, "latency_ms": -1})

@app.route("/api/tunnel/status", methods=["GET"])
def tunnel_status_route():
    is_running = (tunnel_process is not None and tunnel_process.poll() is None and tunnel_url is not None)
    return jsonify({"running": is_running, "url": tunnel_url if is_running else None})

@app.route("/api/tunnel/start", methods=["POST"])
def tunnel_start_route():
    port = int(os.environ.get("PORT", 8000))
    url = start_tunnel_thread(port)
    if url:
        return jsonify({"status": "ok", "url": url})
    return jsonify({"status": "error", "message": "Failed to start Cloudflare tunnel. Run 'pkg install cloudflared' in Termux."})

@app.route("/api/tunnel/stop", methods=["POST"])
def tunnel_stop_route():
    stop_tunnel()
    return jsonify({"status": "ok"})

@app.route("/api/ai/chat", methods=["POST"])
def ai_chat_route():
    data = request.get_json(silent=True) or {}
    msg = data.get("message", "").strip()
    push_to_oled = data.get("push_to_oled", True)
    if not msg:
        return jsonify({"status": "error", "message": "Empty message"})

    emotion, reply = query_ai(msg)
    pushed = False
    pages = 1
    if push_to_oled:
        target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
        try:
            r = requests.post(
                f"http://{target_ip}/api/message",
                data={"text": reply, "emotion": emotion, "title": "BUDDY AI"},
                timeout=3.0
            )
            if r.status_code == 200:
                pushed = True
                pages = r.json().get("pages", 1)
        except Exception as e:
            print(f"[DESKBUDDY] /api/message error: {e}, fallback to /api/photo")
            try:
                hex_data = render_text_to_oled_hex(reply, emotion=emotion, title="BUDDY AI")
                pushed = send_photo_to_deskbuddy(hex_data, save_permanent=False)
            except Exception:
                pass

    return jsonify({
        "status": "ok",
        "emotion": emotion,
        "reply": reply,
        "pushed_to_oled": pushed,
        "pages": pages
    })

# ==============================================================================
# Voice Assistant Engine (STT, Multimodal Audio, Neural TTS)
# ==============================================================================
def transcribe_audio_whisper_groq(audio_bytes: bytes, api_key: str) -> str:
    """Fast speech transcription via Groq Whisper (< 250ms latency)"""
    try:
        url = "https://api.groq.com/openai/v1/audio/transcriptions"
        headers = {"Authorization": f"Bearer {api_key}"}
        files = {"file": ("audio.wav", audio_bytes, "audio/wav")}
        data = {"model": "whisper-large-v3-turbo"}
        r = requests.post(url, headers=headers, files=files, data=data, timeout=8.0)
        if r.status_code == 200:
            return r.json().get("text", "").strip()
        print(f"[VOICE] Groq Whisper error {r.status_code}: {r.text[:80]}")
    except Exception as e:
        print(f"[VOICE] Groq Whisper exception: {e}")
    return ""

def ask_gemini_voice(audio_bytes: bytes, api_key: str) -> tuple[str, str, str]:
    """Single-shot multimodal Gemini Audio: listens and answers in Hinglish/English"""
    try:
        audio_b64 = base64.b64encode(audio_bytes).decode("utf-8")
        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={api_key}"
        prompt = (
            f"{SYSTEM_PROMPT}\n\n"
            "Instructions:\n"
            "1. First transcribe what user said: write 'USER: <transcription>'\n"
            "2. Then write your punchy response prefixed with ONE emotion tag, e.g.: [HAPPY] Hello there!\n"
            "Keep response under 2 short sentences suitable for small OLED screen."
        )
        payload = {
            "contents": [
                {
                    "role": "user",
                    "parts": [
                        {"text": prompt},
                        {"inlineData": {"mimeType": "audio/wav", "data": audio_b64}}
                    ]
                }
            ],
            "generationConfig": {"temperature": 0.7, "maxOutputTokens": 100}
        }
        r = requests.post(url, json=payload, timeout=10.0)
        if r.status_code == 200:
            res = r.json()
            raw = res["candidates"][0]["content"]["parts"][0]["text"].strip()
            user_text = ""
            emotion = "HAPPY"
            reply = raw
            for line in raw.split("\n"):
                line_s = line.strip()
                if line_s.upper().startswith("USER:"):
                    user_text = line_s[5:].strip()
                elif line_s.startswith("[") and "]" in line_s:
                    end_idx = line_s.find("]")
                    emotion = line_s[1:end_idx].strip().upper()
                    reply = line_s[end_idx+1:].strip()
            return user_text, emotion, reply
        print(f"[VOICE] Gemini audio error {r.status_code}: {r.text[:80]}")
    except Exception as e:
        print(f"[VOICE] Gemini audio exception: {e}")
    return "", "ANNOYED", "Could not process audio"

def generate_tts_wav(text: str, voice: str = "hi-IN-SwaraNeural") -> str:
    """Synthesizes text to 16kHz 16-bit Mono WAV for ESP32 MAX98357A speaker"""
    clean_text = re.sub(r"\[.*?\]", "", text).strip()
    if not clean_text:
        clean_text = "Hello!"

    out_filename = f"reply_{int(time.time()*1000)}.wav"
    out_path = AUDIO_DIR / out_filename
    temp_mp3 = AUDIO_DIR / f"temp_{int(time.time()*1000)}.mp3"

    tts_success = False

    # 1. Try high quality Microsoft Edge Neural TTS
    if shutil.which("edge-tts"):
        try:
            cmd = ["edge-tts", "--voice", voice, "--text", clean_text, "--write-media", str(temp_mp3)]
            res = subprocess.run(cmd, timeout=8, capture_output=True)
            if res.returncode == 0 and temp_mp3.exists() and temp_mp3.stat().st_size > 500:
                tts_success = True
        except Exception as e:
            print(f"[TTS] edge-tts error: {e}")

    # 2. Fallback to keyless Google Translate TTS
    if not tts_success:
        try:
            tts_url = f"https://translate.google.com/translate_tts?ie=UTF-8&q={urllib.parse.quote(clean_text[:120])}&tl=hi&client=tw-ob"
            r = requests.get(tts_url, headers={"User-Agent": "Mozilla/5.0"}, timeout=6.0)
            if r.status_code == 200 and len(r.content) > 500:
                with open(temp_mp3, "wb") as f:
                    f.write(r.content)
                tts_success = True
        except Exception as e:
            print(f"[TTS] Google TTS error: {e}")

    # 3. Convert MP3 to 16kHz 16-bit Mono WAV using ffmpeg
    if tts_success and temp_mp3.exists():
        if shutil.which("ffmpeg"):
            try:
                cmd = [
                    "ffmpeg", "-y", "-i", str(temp_mp3),
                    "-ac", "1", "-ar", "16000", "-acodec", "pcm_s16le",
                    str(out_path)
                ]
                subprocess.run(cmd, timeout=8, capture_output=True)
                if out_path.exists() and out_path.stat().st_size > 100:
                    try: temp_mp3.unlink()
                    except Exception: pass
                    return f"/api/voice/audio/{out_filename}"
            except Exception as e:
                print(f"[TTS] ffmpeg error: {e}")

    # Cleanup temp
    if temp_mp3.exists():
        try: temp_mp3.unlink()
        except Exception: pass

    return ""

@app.route("/api/voice/chat", methods=["POST"])
def voice_chat_route():
    audio_file = request.files.get("audio")
    audio_bytes = b""
    if audio_file:
        audio_bytes = audio_file.read()
    elif request.data:
        audio_bytes = request.data

    if not audio_bytes or len(audio_bytes) < 100:
        return jsonify({"status": "error", "message": "No audio received"}), 400

    api_key = cfg.get("gemini_api_key", "").strip()
    voice = cfg.get("voice", "hi-IN-SwaraNeural")

    user_text = ""
    emotion = "HAPPY"
    reply = ""

    # Priority 1: Groq Whisper STT + Fast LLM
    if api_key.startswith("gsk_"):
        user_text = transcribe_audio_whisper_groq(audio_bytes, api_key)
        if not user_text:
            user_text = "Hello DeskBuddy"
        emotion, reply = query_ai(user_text)
    # Priority 2: Gemini Direct Multimodal Audio
    elif api_key:
        user_text, emotion, reply = ask_gemini_voice(audio_bytes, api_key)
        if not reply or reply.startswith("Could not process") or reply.startswith("Gemini Error"):
            emotion, reply = query_ai("Hello DeskBuddy")
    else:
        user_text = "Namaste"
        emotion = "HAPPY"
        reply = "Namaste! Please add your Gemini or Groq API key in Settings to unlock my voice brain!"

    # Synthesize Neural Speech Audio
    audio_url = generate_tts_wav(reply, voice=voice)

    return jsonify({
        "status": "ok",
        "user_text": user_text,
        "emotion": emotion,
        "reply": reply,
        "audio_url": audio_url
    })

@app.route("/api/voice/audio/<path:filename>", methods=["GET"])
def voice_audio_serve(filename):
    return send_from_directory(AUDIO_DIR, filename, mimetype="audio/wav")

@app.route("/api/media/upload-photo", methods=["POST"])
def upload_photo_route():
    f = request.files.get("file")
    if not f:
        return jsonify({"status": "error", "message": "No file uploaded"})
    dither = request.form.get("dither", "floyd")
    invert = request.form.get("invert", "false").lower() == "true"
    save = request.form.get("save", "true").lower() == "true"

    try:
        img = Image.open(f.stream)
        hex_str = pil_to_ssd1306_hex(img, dither_mode=dither, invert=invert)
        ok = send_photo_to_deskbuddy(hex_str, save_permanent=save)
        return jsonify({"status": "ok" if ok else "failed", "pushed": ok})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)})

@app.route("/api/media/upload-video", methods=["POST"])
def upload_video_route():
    f = request.files.get("file")
    if not f:
        return jsonify({"status": "error", "message": "No video file"})

    fps = int(request.form.get("fps", 10))
    max_duration = int(request.form.get("max_duration", 8))
    dither = request.form.get("dither", "floyd")
    invert = request.form.get("invert", "false").lower() == "true"

    if fps <= 0 or fps > 20: fps = 10
    if max_duration <= 0 or max_duration > 10: max_duration = 8
    target_frames = min(100, fps * max_duration)

    suffix = Path(f.filename).suffix or ".mp4"
    with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as tmp_in:
        f.save(tmp_in)
        input_file = tmp_in.name

    out_dir = tempfile.mkdtemp()
    try:
        ffmpeg_bin = shutil.which("ffmpeg")
        if not ffmpeg_bin:
            return jsonify({"status": "error", "message": "FFmpeg not found! Run 'pkg install ffmpeg' in Termux."}), 500

        out_pattern = os.path.join(out_dir, "frame_%04d.png")
        cmd = [
            ffmpeg_bin, "-y", "-i", input_file,
            "-t", str(max_duration),
            "-vf", f"fps={fps},scale=128:64:force_original_aspect_ratio=increase,crop=128:64",
            out_pattern
        ]
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.returncode != 0:
            return jsonify({"status": "error", "message": "FFmpeg conversion failed"}), 500

        frame_files = sorted(Path(out_dir).glob("frame_*.png"))[:target_frames]
        if not frame_files:
            return jsonify({"status": "error", "message": "No frames extracted"}), 400

        hex_frames = []
        for fp in frame_files:
            with Image.open(fp) as f_img:
                hex_frames.append(pil_to_ssd1306_hex(f_img, dither_mode=dither, invert=invert))

        target_ip = cfg.get("deskbuddy_ip", "10.91.8.125")
        start_res = requests.post(
            f"http://{target_ip}/api/video/start",
            data={"frames": len(hex_frames), "fps": fps},
            timeout=3.0
        ).json()
        if start_res.get("status") != "ok":
            return jsonify({"status": "error", "message": "DeskBuddy rejected video start"})

        for idx, h_data in enumerate(hex_frames):
            requests.post(
                f"http://{target_ip}/api/video/frame",
                data={"idx": idx, "data": h_data},
                timeout=2.0
            )

        requests.post(f"http://{target_ip}/api/video/end?save=1", timeout=3.0)
        return jsonify({
            "status": "ok",
            "frames_uploaded": len(hex_frames),
            "fps": fps,
            "duration": round(len(hex_frames) / fps, 1)
        })
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)})
    finally:
        if os.path.exists(input_file): os.unlink(input_file)
        shutil.rmtree(out_dir, ignore_errors=True)

# ==============================================================================
# Web UI Dashboard (Mobile First, Beautiful Modern Dark Theme)
# ==============================================================================
@app.route("/")
def index_page():
    return render_template_string("""<!DOCTYPE html>
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

  <!-- GLOBAL CLOUDFLARE TUNNEL BANNER -->
  <div style="margin-bottom:14px; padding:10px 14px; border-radius:10px; background:#161c27; border:1px solid #30363d; display:flex; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:8px;">
    <div style="display:flex; align-items:center; gap:8px;">
      <span style="font-size:18px;">🌐</span>
      <div>
        <div style="font-size:12px; font-weight:700; color:#fff;">Global Cloudflare Access</div>
        <div style="font-size:11px; color:#8b949e;" id="tunnelStatusText">Checking tunnel...</div>
      </div>
    </div>
    <div style="display:flex; gap:6px;">
      <button type="button" id="tunnelBtn" onclick="toggleTunnel()" style="padding:6px 12px; border-radius:6px; font-size:11px; font-weight:700; cursor:pointer; background:var(--accent); color:#000; border:none;">🌐 Go Global</button>
      <button type="button" id="copyTunnelBtn" onclick="copyTunnelUrl()" style="display:none; padding:6px 10px; border-radius:6px; font-size:11px; font-weight:600; cursor:pointer; background:#21262d; color:#fff; border:1px solid #30363d;">📋 Copy</button>
    </div>
  </div>

  <div class="tabs">
    <div class="tab-btn active" onclick="switchTab('chat')">💬 AI Chat</div>
    <div class="tab-btn" onclick="switchTab('voice')">🎙️ Voice AI</div>
    <div class="tab-btn" onclick="switchTab('media')">🎬 Media</div>
    <div class="tab-btn" onclick="switchTab('settings')">⚙️ Settings</div>
  </div>

  <!-- TAB: AI CHAT -->
  <div id="tab-chat" class="tab-pane active">
    <div class="card">
      <h2>🤖 Chat with DeskBuddy AI (Gemini / Groq)</h2>
      <div class="chat-box" id="chatBox">
        <div class="msg msg-bot"><b>DeskBuddy:</b> Hi! I'm your AI buddy. Ask me anything, and I'll display the answer on my screen!</div>
      </div>
      <input type="text" id="chatInput" placeholder="Ask a question or say hi..." onkeydown="if(event.key==='Enter') sendChat()">
      <button class="btn" id="sendChatBtn" onclick="sendChat()">⚡ Ask & Show on DeskBuddy</button>
    </div>
  </div>

  <!-- TAB: VOICE AI -->
  <div id="tab-voice" class="tab-pane">
    <div class="card">
      <h2>🎙️ Real-Time Voice Assistant (I2S Mic &amp; Speaker)</h2>
      <p style="font-size:12px; color:var(--text-muted); margin-bottom:12px;">
        DeskBuddy listens with INMP441, streams voice to this phone server, and answers aloud through MAX98357A speaker!
      </p>
      <div style="background:#0e121a; border:1px solid var(--border); border-radius:10px; padding:14px; text-align:center; margin-bottom:12px;">
        <div id="voiceStatus" style="font-size:14px; font-weight:700; color:var(--accent); margin-bottom:8px;">Ready to Talk</div>
        <div id="voiceTranscript" style="font-size:12px; color:#c9d1d9; min-height:36px; margin-bottom:10px;">Speak directly to DeskBuddy's microphone, or test from phone browser below:</div>
        <button type="button" class="btn btn-blue" id="phoneMicBtn" onclick="togglePhoneRecord()">🎙️ Test Voice from Phone Mic</button>
        <audio id="ttsAudioPlayer" style="display:none; width:100%; margin-top:12px;" controls></audio>
      </div>
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

      <label style="font-size:12px; color:var(--text-muted);">AI API Key (Gemini or Groq):</label>
      <input type="password" id="cfgKey" placeholder="AIzaSy... or gsk_...">
      <p style="font-size:11px; color:var(--text-muted); margin-bottom:12px;">Google Gemini keys start with <b>AIzaSy...</b>. You can also use ultra-fast 100% free Groq keys (<b>gsk_...</b>) from <a href="https://console.groq.com/keys" target="_blank" style="color:var(--accent);">console.groq.com</a>.</p>

      <label style="font-size:12px; color:var(--text-muted);">Voice Neural Character:</label>
      <select id="cfgVoice">
        <option value="hi-IN-SwaraNeural">Swara (Hindi Female - Sweet)</option>
        <option value="hi-IN-MadhurNeural">Madhur (Hindi Male - Natural)</option>
        <option value="en-IN-NeerjaNeural">Neerja (Indian English Female)</option>
        <option value="en-US-JennyNeural">Jenny (English US Female)</option>
      </select>

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
    if (d.voice) document.getElementById('cfgVoice').value = d.voice;
  }
  loadSettings();

  async function saveSettings() {
    const ip = document.getElementById('cfgIp').value.trim();
    const key = document.getElementById('cfgKey').value.trim();
    const voice = document.getElementById('cfgVoice').value;
    const r = await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ deskbuddy_ip: ip, gemini_api_key: key, voice: voice })
    });
    if (r.ok) {
      showToast('Settings saved!');
      checkPing();
    }
  }

  // Phone browser test microphone recording via Web Audio
  let mediaRecorder = null;
  let audioChunks = [];
  let isPhoneRecording = false;

  async function togglePhoneRecord() {
    const btn = document.getElementById('phoneMicBtn');
    const status = document.getElementById('voiceStatus');
    const transcript = document.getElementById('voiceTranscript');
    const player = document.getElementById('ttsAudioPlayer');

    if (!isPhoneRecording) {
      try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        mediaRecorder = new MediaRecorder(stream);
        audioChunks = [];
        mediaRecorder.ondataavailable = e => { if (e.data.size > 0) audioChunks.push(e.data); };
        mediaRecorder.onstop = async () => {
          const audioBlob = new Blob(audioChunks, { type: 'audio/wav' });
          status.innerText = '🧠 Thinking & Synthesizing Speech...';
          btn.innerText = '⏳ Processing...';
          btn.disabled = true;

          const fd = new FormData();
          fd.append('audio', audioBlob, 'phone_mic.wav');

          try {
            const res = await fetch('/api/voice/chat', { method: 'POST', body: fd });
            const data = await res.json();
            btn.disabled = false;
            btn.innerText = '🎙️ Test Voice from Phone Mic';
            if (data.status === 'ok') {
              status.innerText = `DeskBuddy [${data.emotion}]:`;
              transcript.innerText = `"${data.reply}"`;
              if (data.audio_url) {
                player.src = data.audio_url;
                player.style.display = 'block';
                player.play();
              }
              showToast('Voice processed! Speaking on DeskBuddy!', '#00C853');
            } else {
              status.innerText = 'Error: ' + (data.message || 'Voice error');
            }
          } catch(err) {
            btn.disabled = false;
            btn.innerText = '🎙️ Test Voice from Phone Mic';
            status.innerText = 'Error: ' + err.message;
          }
        };

        mediaRecorder.start();
        isPhoneRecording = true;
        btn.innerText = '🛑 Stop & Ask DeskBuddy';
        btn.style.background = '#FF5252';
        status.innerText = '🎙️ Listening to you... Speak now!';
      } catch(err) {
        alert('Microphone access denied: ' + err.message);
      }
    } else {
      if (mediaRecorder && mediaRecorder.state !== 'inactive') {
        mediaRecorder.stop();
      }
      isPhoneRecording = false;
      btn.style.background = 'var(--accent-blue)';
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
        const pageTxt = (d.pages && d.pages > 1) ? ` (${d.pages} Pages • Tap DeskBuddy for next)` : '';
        showToast('Sent to DeskBuddy OLED!' + pageTxt, '#00C853');
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

  // Cloudflare Tunnel Management
  let currentTunnelUrl = null;
  async function checkTunnelStatus() {
    try {
      const r = await fetch('/api/tunnel/status');
      const d = await r.json();
      const sub = document.getElementById('tunnelStatusText');
      const btn = document.getElementById('tunnelBtn');
      const copyBtn = document.getElementById('copyTunnelBtn');
      if (d.running && d.url) {
        currentTunnelUrl = d.url;
        sub.innerHTML = `<a href="${d.url}" target="_blank" style="color:#00E5FF; font-weight:700; text-decoration:none;">${d.url}</a>`;
        btn.innerText = 'Stop';
        btn.style.background = '#ff4b4b';
        btn.style.color = '#fff';
        copyBtn.style.display = 'inline-block';
      } else {
        currentTunnelUrl = null;
        sub.innerText = 'Offline (Accessible only on local Wi-Fi)';
        btn.innerText = '🌐 Go Global';
        btn.style.background = 'var(--accent)';
        btn.style.color = '#000';
        copyBtn.style.display = 'none';
      }
    } catch(e) {}
  }

  async function toggleTunnel() {
    const btn = document.getElementById('tunnelBtn');
    btn.disabled = true;
    if (currentTunnelUrl) {
      btn.innerText = 'Stopping...';
      await fetch('/api/tunnel/stop', { method: 'POST' });
    } else {
      btn.innerText = 'Starting...';
      const r = await fetch('/api/tunnel/start', { method: 'POST' });
      const d = await r.json();
      if (d.status === 'ok') {
        showToast('🎉 Cloudflare Tunnel Live!', '#00C853');
      } else {
        alert(d.message || 'Tunnel error');
      }
    }
    btn.disabled = false;
    await checkTunnelStatus();
  }

  function copyTunnelUrl() {
    if (currentTunnelUrl) {
      navigator.clipboard.writeText(currentTunnelUrl);
      showToast('📋 Public URL copied!', '#00C853');
    }
  }

  checkTunnelStatus();
  setInterval(checkTunnelStatus, 8000);
</script>
</body>
</html>
""")

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8000))
    local_ip = get_local_ip()
    auto_tunnel = ("--tunnel" in sys.argv) or os.environ.get("AUTO_TUNNEL") == "1" or cfg.get("auto_tunnel", False)

    print("=" * 65)
    print("🤖 DeskBuddy Smart Brain Server Running!")
    print(f"👉 Local Access:  http://localhost:{port}")
    print(f"👉 Wi-Fi Access:  http://{local_ip}:{port}")
    if auto_tunnel:
        print("🌐 Starting Cloudflare Global Tunnel...")
        t_thread = threading.Thread(target=start_tunnel_thread, args=(port,), daemon=True)
        t_thread.start()
    else:
        print("💡 Tip: Click 'Go Global' in Web UI or run with --tunnel for worldwide access!")
    print("=" * 65)
    app.run(host="0.0.0.0", port=port, debug=False)
