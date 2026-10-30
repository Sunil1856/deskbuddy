# 🤖 DeskBuddy Smart Brain Server (Android Termux & Cloud)

DeskBuddy ka high-speed AI & Media Processing engine jo aapke **purane Android phone (bina root ke)** par 24/7 chalta hai.

---

### ✨ Features:
1. **Google Gemini 1.5 Flash AI:**
   - Real-time Q&A aur robot conversation.
   - Screen ke hisaab se short, witty answers + emotional reactions.
   - Live answer DeskBuddy ke OLED screen par push hota hai!
2. **Instant Video & Photo Converter (FFmpeg In-Memory):**
   - Koi bhi heavy video (MP4, WebM, GIF, 1080p) drop karein.
   - Phone ka CPU second ke 100ve hisse me 128x64 monochrome frames me convert karke DeskBuddy ko flash kar deta hai.
3. **Cloudflare Global Tunnel (Optional):**
   - Phone ghar me charger par laga rahega, aur DeskBuddy pocket me college ya bahar kahin bhi chalega.
4. **Future-Ready Voice Engine:**
   - Edge-TTS integrated for natural Hindi/English voice output.

---

### 📱 Android Phone Par 2 Minute Setup Guide (No Root Needed!):

#### Step 1: Termux Install Karein
Apne phone par **Termux** app install karein:
* [F-Droid Termux Download](https://f-droid.org/packages/com.termux/) (Recommended)

#### Step 2: DeskBuddy Server Download & Setup
Termux open karke bas ye commands type karein:
```bash
git clone https://github.com/Sunil1856/deskbuddy.git
cd deskbuddy/deskbuddy-server
chmod +x setup_termux.sh start.sh tunnel.sh
./setup_termux.sh
```

#### Step 3: Web Dashboard Open Karein
Server start hote hi screen par ek link aayega:
👉 **`http://localhost:8000`** (ya aapke phone ka Wi-Fi IP jaise `http://192.168.1.15:8000`)

Apne phone ke Chrome browser me open karein, **Settings** me jaakar:
1. DeskBuddy ka IP enter karein (e.g. `10.91.8.125`).
2. Google Gemini API Key paste karein ([Free API Key Link](https://aistudio.google.com/app/apikey)).

Bas! Ab aap DeskBuddy se chat kar sakte hain aur heavy videos direct upload kar sakte hain!
