#!/data/data/com.termux/files/usr/bin/bash
# ==============================================================================
# DeskBuddy Cloudflare Global Tunnel (Access from College / Anywhere)
# ==============================================================================
echo "=========================================================="
echo "🌐 Starting Free Global Cloudflare Tunnel for DeskBuddy"
echo "=========================================================="

# Check if cloudflared is installed
if ! command -v cloudflared &> /dev/null; then
    echo "📦 Checking and installing Cloudflared package in Termux..."
    pkg install -y cloudflared 2>/dev/null
    
    # If pkg install did not work, download the standalone binary
    if ! command -v cloudflared &> /dev/null; then
        echo "📦 Downloading Cloudflared static binary for Android..."
        ARCH=$(uname -m)
        PREFIX_BIN="${PREFIX:-/data/data/com.termux/files/usr}/bin"
        mkdir -p "$PREFIX_BIN"
        if [ "$ARCH" = "aarch64" ]; then
            curl -fsSL -o "$PREFIX_BIN/cloudflared" https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64 || wget -q -O "$PREFIX_BIN/cloudflared" https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64
        else
            curl -fsSL -o "$PREFIX_BIN/cloudflared" https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm || wget -q -O "$PREFIX_BIN/cloudflared" https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm
        fi
        chmod +x "$PREFIX_BIN/cloudflared"
    fi
fi

if ! command -v cloudflared &> /dev/null; then
    echo "❌ Error: Could not install cloudflared automatically."
    echo "Please run manually: pkg install cloudflared"
    exit 1
fi

echo "🚀 Connecting to Cloudflare Global Network..."
echo "----------------------------------------------------------"
echo "Copy the 'https://...trycloudflare.com' link below to open"
echo "DeskBuddy from ANY phone, laptop or network in the world!"
echo "----------------------------------------------------------"
cloudflared tunnel --url http://127.0.0.1:8000
