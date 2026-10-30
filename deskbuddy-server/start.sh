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

python3 server.py
