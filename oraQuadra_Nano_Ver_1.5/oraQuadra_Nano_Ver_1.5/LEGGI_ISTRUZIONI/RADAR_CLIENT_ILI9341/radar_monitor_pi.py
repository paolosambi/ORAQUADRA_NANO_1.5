#!/usr/bin/env python3
"""
Radar Monitor Controller per Raspberry Pi 4
Riceve comandi ON/OFF dal RADAR_CLIENT_ILI9341 e controlla il monitor HDMI.

Installazione sul Raspberry Pi:
  1. Copia questo file sul Pi: scp radar_monitor_pi.py pi@192.168.1.122:~/
  2. Avvia: sudo python3 radar_monitor_pi.py
  3. Per avvio automatico al boot:
     sudo cp radar_monitor_pi.py /usr/local/bin/
     sudo cp radar_monitor_pi.service /etc/systemd/system/
     sudo systemctl enable radar_monitor_pi
     sudo systemctl start radar_monitor_pi

Endpoint esposti (porta 80):
  GET /radar/on          -> Accende monitor
  GET /radar/off         -> Spegne monitor
  GET /radar/ping        -> Info dispositivo
  GET /radar/presence    -> Gestione presenza (value=true/false)
  GET /radar/brightness  -> Riceve luminosita (ignorato)
  GET /radar/temperature -> Riceve temperatura (ignorato)
  GET /radar/humidity    -> Riceve umidita (ignorato)
  GET /radar/test        -> Test connessione
"""

import subprocess
import json
import time
import socket
import os
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

DEVICE_NAME = "RaspberryPi4"
PORT = 80
monitor_on = True


def get_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "unknown"


def set_monitor(on):
    """Accende o spegne il monitor HDMI."""
    global monitor_on
    state = "1" if on else "0"

    # Metodo 1: vcgencmd (Raspberry Pi OS)
    try:
        subprocess.run(
            ["vcgencmd", "display_power", state],
            check=True, timeout=5, capture_output=True
        )
        monitor_on = on
        print(f"[MONITOR] {'ON' if on else 'OFF'} (vcgencmd)")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Metodo 2: xrandr (X11)
    try:
        if on:
            subprocess.run(
                ["xrandr", "--output", "HDMI-1", "--auto"],
                check=True, timeout=5, capture_output=True,
                env={**os.environ, "DISPLAY": ":0"}
            )
        else:
            subprocess.run(
                ["xrandr", "--output", "HDMI-1", "--off"],
                check=True, timeout=5, capture_output=True,
                env={**os.environ, "DISPLAY": ":0"}
            )
        monitor_on = on
        print(f"[MONITOR] {'ON' if on else 'OFF'} (xrandr HDMI-1)")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Metodo 3: wlr-randr (Wayland)
    try:
        displays = subprocess.run(
            ["wlr-randr"], capture_output=True, text=True, timeout=5
        )
        # Prende il primo output trovato
        for line in displays.stdout.splitlines():
            if line and not line.startswith(" "):
                output_name = line.split()[0]
                if on:
                    subprocess.run(
                        ["wlr-randr", "--output", output_name, "--on"],
                        check=True, timeout=5, capture_output=True
                    )
                else:
                    subprocess.run(
                        ["wlr-randr", "--output", output_name, "--off"],
                        check=True, timeout=5, capture_output=True
                    )
                monitor_on = on
                print(f"[MONITOR] {'ON' if on else 'OFF'} (wlr-randr {output_name})")
                return True
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Metodo 4: tvservice (vecchio Raspberry Pi OS)
    try:
        if on:
            subprocess.run(
                ["tvservice", "-p"],
                check=True, timeout=5, capture_output=True
            )
            # Dopo tvservice -p serve reinizializzare il framebuffer
            subprocess.run(
                ["fbset", "-depth", "8"], timeout=5, capture_output=True
            )
            subprocess.run(
                ["fbset", "-depth", "16"], timeout=5, capture_output=True
            )
        else:
            subprocess.run(
                ["tvservice", "-o"],
                check=True, timeout=5, capture_output=True
            )
        monitor_on = on
        print(f"[MONITOR] {'ON' if on else 'OFF'} (tvservice)")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        pass

    print("[MONITOR] ERRORE: nessun metodo funziona!")
    return False


class RadarHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        params = parse_qs(parsed.query)

        if path == "/radar/on":
            set_monitor(True)
            self.respond({"status": "ok", "display": "on"})

        elif path == "/radar/off":
            set_monitor(False)
            self.respond({"status": "ok", "display": "off"})

        elif path == "/radar/presence":
            value = params.get("value", [""])[0]
            if value == "true":
                set_monitor(True)
            elif value == "false":
                set_monitor(False)
            self.respond({"status": "ok", "presence": value})

        elif path == "/radar/ping":
            self.respond({
                "name": DEVICE_NAME,
                "type": "raspberry-pi",
                "ip": get_ip(),
                "monitor": monitor_on
            })

        elif path == "/radar/test":
            self.respond({"status": "ok", "name": DEVICE_NAME, "monitor": monitor_on})

        elif path in ("/radar/brightness", "/radar/temperature", "/radar/humidity"):
            # Riceve dati ambientali dal radar - accetta silenziosamente
            self.respond({"status": "ok"})

        elif path == "/api/status":
            # Per discovery dal RADAR_CLIENT
            self.respond({
                "name": DEVICE_NAME,
                "type": "raspberry-pi",
                "ip": get_ip(),
                "monitor": monitor_on,
                "uptime": int(time.time() - start_time)
            })

        else:
            self.send_response(404)
            self.end_headers()

    def respond(self, data):
        body = json.dumps(data).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        print(f"[HTTP] {args[0]}")


if __name__ == "__main__":
    start_time = time.time()
    ip = get_ip()

    print("=" * 50)
    print("  Radar Monitor Controller - Raspberry Pi 4")
    print("=" * 50)
    print(f"  IP: {ip}")
    print(f"  Porta: {PORT}")
    print(f"  Endpoint: /radar/on, /radar/off, /radar/ping")
    print("=" * 50)

    server = HTTPServer(("0.0.0.0", PORT), RadarHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nArresto...")
        server.server_close()
