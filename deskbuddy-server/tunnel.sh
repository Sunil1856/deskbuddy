#!/data/data/com.termux/files/usr/bin/bash
# ==============================================================================
# DeskBuddy Cloudflare Global Tunnel (Work from College / Anywhere)
# ==============================================================================
echo "=========================================================="
echo "🌐 Starting Free Global Cloudflare Tunnel for DeskBuddy"
echo "=========================================================="

# Check if cloudflared is installed
if ! command -v cloudflared &> /dev/null; then
    echo "📦 Downloading Cloudflared for Android (ARM64)..."
    ARCH=$(uname -m)
    if [ "$ARCH" = "aarch64" ]; then
        wget -q -O $PREFIX/bin/cloudflared https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64
    else
        wget -q -O $PREFIX/bin/cloudflared https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm
    fi
    chmod +x $PREFIX/bin/cloudflared
fi

echo "🚀 Connecting to Cloudflare Network..."
echo "Copy the 'https://...trycloudflare.com' link shown below to access DeskBuddy from anywhere in the world!"
echo "=========================================================="
cloudflared tunnel --url http://localhost:8000
