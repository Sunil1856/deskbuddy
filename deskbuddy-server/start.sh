#!/data/data/com.termux/files/usr/bin/bash
# ==============================================================================
# DeskBuddy Brain Server - Start Script
# ==============================================================================
cd "$(dirname "$0")"

# Keep phone awake in background
termux-wake-lock 2>/dev/null

echo "=========================================================="
echo "🤖 Starting DeskBuddy Brain Server..."
echo "=========================================================="

# Find Phone Local IP
LOCAL_IP=$(ip -4 addr show wlan0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | head -n 1)
if [ -z "$LOCAL_IP" ]; then
  LOCAL_IP="localhost"
fi

echo ""
echo "📱 Open this link in your Phone's Chrome Browser:"
echo "👉 http://${LOCAL_IP}:8000"
echo "👉 http://localhost:8000"
echo ""
echo "Press Ctrl+C to stop the server."
echo "=========================================================="

python3 server.py
