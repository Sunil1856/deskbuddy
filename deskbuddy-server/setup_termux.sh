#!/data/data/com.termux/files/usr/bin/bash
# ==============================================================================
# DeskBuddy Brain Server - 1-Click Termux Setup Script (Android Phone)
# ==============================================================================
echo "=========================================================="
echo "🤖 Setting up DeskBuddy AI & Media Brain on Android Termux"
echo "=========================================================="

# 1. Acquire Wakelock so phone does not sleep when screen is locked
termux-wake-lock 2>/dev/null

# 2. Update Termux Packages
echo "📦 Updating Termux package lists..."
pkg update -y

# 3. Install Python, FFmpeg, Pillow, Git
echo "📦 Installing Python, FFmpeg & System dependencies..."
pkg install -y python python-pillow ffmpeg git wget curl

# 4. Install Python Requirements
echo "🐍 Installing Python Libraries..."
pip install --upgrade pip
pip install fastapi uvicorn requests python-multipart edge-tts

# 5. Make start script executable
chmod +x start.sh 2>/dev/null

echo ""
echo "=========================================================="
echo "🎉 Setup Complete! To start the server anytime, just run:"
echo "   ./start.sh"
echo "=========================================================="
./start.sh
