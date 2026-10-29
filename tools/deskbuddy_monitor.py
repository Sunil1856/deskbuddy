#!/usr/bin/env python3
"""
DeskBuddy — Laptop & PC Hardware Monitor Server
Serves live CPU, RAM, and Network stats to your ESP32 DeskBuddy OLED screen over Wi-Fi.
Port: 34001
"""
import sys
import os
import time
import json
import socket
from http.server import HTTPServer, BaseHTTPRequestHandler

# Check for psutil
try:
    import psutil
except ImportError:
    print("\n[!] 'psutil' library not found.")
    print("[*] Installing psutil automatically...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "psutil"])
    import psutil

# Track previous network counters for speed calculation
_prev_net = psutil.net_io_counters()
_prev_time = time.time()

def get_local_ip():
    """Find local network IPv4 address."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

def get_system_metrics():
    """Collect real-time CPU, RAM, and Network transfer rates."""
    global _prev_net, _prev_time
    
    # CPU & RAM Usage
    cpu_percent = psutil.cpu_percent(interval=None)
    ram = psutil.virtual_memory()
    ram_percent = ram.percent

    # Network Speed (Bytes/sec)
    curr_time = time.time()
    curr_net = psutil.net_io_counters()
    time_delta = max(curr_time - _prev_time, 0.001)
    
    dn_speed = int((curr_net.bytes_recv - _prev_net.bytes_recv) / time_delta)
    up_speed = int((curr_net.bytes_sent - _prev_net.bytes_sent) / time_delta)
    
    _prev_net = curr_net
    _prev_time = curr_time

    # CPU Temperature (OS-dependent, fallback to simulated temp based on CPU load)
    cpu_temp = 42.0 + (cpu_percent * 0.35)
    try:
        if hasattr(psutil, "sensors_temperatures"):
            temps = psutil.sensors_temperatures()
            if temps:
                for name, entries in temps.items():
                    if entries:
                        cpu_temp = entries[0].current
                        break
    except Exception:
        pass

    # Simulated Fan speed (RPM estimated from CPU load if hardware sensors locked)
    fan_speed = int(1200 + (cpu_percent * 25))

    return {
        "cpu_temperature": round(cpu_temp, 1),
        "gpu_temperature": round(cpu_temp - 3.0, 1),
        "cpu_percent": round(cpu_percent, 1),
        "memory_percent": round(ram_percent, 1),
        "pwm_fan_speed": fan_speed,
        "network_download_speed": max(dn_speed, 0),
        "network_upload_speed": max(up_speed, 0)
    }

class DeskBuddyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        metrics = get_system_metrics()
        response = {
            "status": "ok",
            "data": [metrics]
        }
        data_bytes = json.dumps(response).encode("utf-8")
        
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data_bytes)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data_bytes)

    def log_message(self, format, *args):
        # Suppress noisy HTTP request logging in terminal
        pass

def main():
    port = 34001
    local_ip = get_local_ip()
    server_address = ("0.0.0.0", port)
    
    # Initialize CPU measurement
    psutil.cpu_percent(interval=None)
    time.sleep(0.1)

    try:
        httpd = HTTPServer(server_address, DeskBuddyHandler)
    except OSError as e:
        print(f"[!] Error: Could not bind to port {port}. Is another instance running?")
        sys.exit(1)

    print("=" * 60)
    print("  💻 DeskBuddy — Laptop Hardware Monitor Server")
    print("=" * 60)
    print(f" [✓] Status      : Running (Listening on all interfaces)")
    print(f" [✓] Laptop IP   : http://{local_ip}:{port}")
    print(f" [✓] Endpoint    : http://{local_ip}:{port}/api/v1.0/get-history?n=1")
    print("-" * 60)
    print(" DeskBuddy ESP32 will now fetch live CPU, RAM & Network stats!")
    print(" Press Ctrl + C to stop.")
    print("=" * 60)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[*] Stopping DeskBuddy Monitor Server...")
        httpd.server_close()

if __name__ == "__main__":
    main()
