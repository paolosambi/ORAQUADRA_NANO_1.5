#!/usr/bin/env python3
"""
AUTO-INSTALLER RADAR SERVICE
=============================
Installa automaticamente il servizio radar su tutti i device
scoperti dal RADAR_CLIENT che non hanno ancora il radar.

Uso:
  python auto_install_radar.py              # Installa su tutti i device [NO RADAR]
  python auto_install_radar.py --watch      # Modalita daemon: controlla ogni 60 sec
  python auto_install_radar.py --ip 1.2.3.4 # Installa su un device specifico

Requisiti: pip install paramiko requests
"""
import sys
import time
import argparse

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

try:
    import paramiko
except ImportError:
    print("[ERRORE] Installa paramiko: pip install paramiko")
    sys.exit(1)

try:
    import requests
except ImportError:
    print("[ERRORE] Installa requests: pip install requests")
    sys.exit(1)

# ===== CONFIGURAZIONE =====
RADAR_CLIENT_IP = "192.168.1.99"   # IP del RADAR_CLIENT ESP32
SSH_USERNAME = "pi"                 # Username SSH default
SSH_PASSWORD = "RASPBERRY"          # Password SSH default
SSH_TIMEOUT = 10                    # Timeout connessione SSH (sec)
# ===========================


def get_devices(radar_ip):
    """Ottieni lista device dal RADAR_CLIENT."""
    try:
        r = requests.get(f"http://{radar_ip}/api/devices", timeout=5)
        data = r.json()
        return data.get('devices', [])
    except Exception as e:
        print(f"[ERRORE] Impossibile contattare RADAR_CLIENT ({radar_ip}): {e}")
        return []


def get_install_script(radar_ip):
    """Scarica lo script di installazione dal RADAR_CLIENT."""
    try:
        r = requests.get(f"http://{radar_ip}/api/install-script", timeout=5)
        return r.text
    except Exception as e:
        print(f"[ERRORE] Impossibile scaricare install-script: {e}")
        return None


def verify_device(radar_ip, index):
    """Chiedi al RADAR_CLIENT di verificare un device."""
    try:
        r = requests.get(f"http://{radar_ip}/api/devices/verify?index={index}", timeout=10)
        return r.json()
    except:
        return {}


def install_on_device(device_ip, device_port, radar_ip, username=SSH_USERNAME, password=SSH_PASSWORD):
    """Installa il servizio radar su un device via SSH."""
    print(f"\n{'='*50}")
    print(f"  INSTALLAZIONE SU {device_ip}:{device_port}")
    print(f"{'='*50}")

    # Connessione SSH
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    try:
        print(f"  [1/4] Connessione SSH a {device_ip}...")
        ssh.connect(device_ip, username=username, password=password, timeout=SSH_TIMEOUT)
        print(f"  [OK]  Connesso come {username}")
    except paramiko.AuthenticationException:
        print(f"  [ERRORE] Autenticazione fallita ({username}/{password})")
        print(f"           Prova con --user e --pass diversi")
        return False
    except Exception as e:
        print(f"  [ERRORE] Connessione SSH fallita: {e}")
        return False

    def run(cmd, timeout=30):
        stdin, stdout, stderr = ssh.exec_command(cmd, timeout=timeout)
        stdout.channel.recv_exit_status()
        out = stdout.read().decode('utf-8', errors='replace').strip()
        err = stderr.read().decode('utf-8', errors='replace').strip()
        return out, err

    # Verifica se radar gia installato
    print(f"  [2/4] Verifico se radar gia installato...")
    out, _ = run(f"curl -s --connect-timeout 2 http://127.0.0.1:{device_port}/radar/ping 2>/dev/null")
    if '"status"' in out and '"ok"' in out:
        print(f"  [OK]  Radar gia attivo! Verifico solo avahi...")
        # Installa comunque avahi se mancante
        out_avahi, _ = run("ls /etc/avahi/services/radardevice.service 2>/dev/null")
        if not out_avahi:
            print(f"  [3/4] Installo servizio avahi mDNS...")
            install_avahi(ssh, run, device_port)
        else:
            print(f"  [OK]  Avahi gia configurato")
        ssh.close()
        return True

    # Scarica e installa
    print(f"  [3/4] Installo radar service (porta {device_port})...")
    install_cmd = f"curl -s http://{radar_ip}/api/install-script | sudo RADAR_PORT={device_port} bash"
    out, err = run(install_cmd, timeout=60)

    if out:
        # Mostra solo le righe importanti
        for line in out.split('\n'):
            if line.strip() and ('===' in line or '[' in line or 'Radar' in line or 'curl' in line):
                print(f"         {line.strip()}")

    # Verifica
    print(f"  [4/4] Verifica installazione...")
    time.sleep(3)
    out, _ = run(f"curl -s --connect-timeout 5 http://127.0.0.1:{device_port}/radar/ping 2>/dev/null")
    if '"ok"' in out:
        print(f"  [OK]  Radar service ATTIVO su porta {device_port}")
        ssh.close()
        return True
    else:
        print(f"  [WARN] Verifica fallita, risposta: {out}")
        # Controlla il processo
        out2, _ = run(f"pgrep -a -f 'radar_service'")
        print(f"         Processi: {out2 or 'nessuno'}")
        out3, _ = run(f"sudo systemctl status radar-service 2>&1 | head -5")
        print(f"         Service: {out3}")
        ssh.close()
        return False

    ssh.close()
    return False


def install_avahi(ssh, run, port):
    """Installa solo il servizio avahi mDNS."""
    hostname, _ = run("hostname")
    avahi_xml = f"""<?xml version="1.0" standalone='no'?>
<!DOCTYPE service-group SYSTEM "avahi-service.dtd">
<service-group>
  <name>{hostname} Radar Service</name>
  <service>
    <type>_radardevice._tcp</type>
    <port>{port}</port>
    <txt-record>name={hostname}</txt-record>
  </service>
</service-group>"""

    run(f"echo '{avahi_xml}' | sudo tee /etc/avahi/services/radardevice.service > /dev/null")
    run("sudo systemctl restart avahi-daemon 2>/dev/null; true")
    print(f"  [OK]  Avahi configurato: {hostname}.local:{port}")


def auto_install_all(radar_ip, username, password, target_ip=None):
    """Installa su tutti i device non verificati."""
    print(f"\n  RADAR_CLIENT: {radar_ip}")
    print(f"  SSH: {username}/{'*' * len(password)}\n")

    devices = get_devices(radar_ip)
    if not devices:
        print("  Nessun device trovato.")
        return 0

    # Filtra device da installare
    if target_ip:
        to_install = [d for d in devices if d['ip'] == target_ip]
        if not to_install:
            print(f"  Device {target_ip} non trovato nella lista.")
            return 0
    else:
        to_install = [d for d in devices if not d.get('radarVerified', False)]

    if not to_install:
        print("  Tutti i device hanno gia il radar installato!")
        # Mostra stato
        for d in devices:
            status = "OK" if d.get('radarVerified') else "NO RADAR"
            print(f"    [{status:>8}] {d['name']} - {d['ip']}:{d['port']}")
        return 0

    print(f"  Device da installare: {len(to_install)}")
    for d in to_install:
        print(f"    -> {d['name']} ({d['ip']}:{d['port']})")

    # Installa su ciascuno
    installed = 0
    for d in to_install:
        ok = install_on_device(d['ip'], d['port'], radar_ip, username, password)
        if ok:
            installed += 1
            # Aggiorna stato su RADAR_CLIENT
            result = verify_device(radar_ip, d['index'])
            if result.get('radarVerified'):
                print(f"  [SYNC] RADAR_CLIENT aggiornato: {d['name']} = VERIFIED")
            else:
                print(f"  [WARN] RADAR_CLIENT non ha verificato: riprova da web UI")

    print(f"\n{'='*50}")
    print(f"  RISULTATO: {installed}/{len(to_install)} installati")
    print(f"{'='*50}\n")
    return installed


def watch_mode(radar_ip, username, password, interval=60):
    """Modalita daemon: controlla periodicamente per nuovi device."""
    print(f"\n  MODALITA WATCH - Controllo ogni {interval} secondi")
    print(f"  Premi Ctrl+C per uscire\n")

    while True:
        try:
            devices = get_devices(radar_ip)
            to_install = [d for d in devices if not d.get('radarVerified', False)]

            if to_install:
                print(f"\n[{time.strftime('%H:%M:%S')}] Trovati {len(to_install)} device senza radar!")
                auto_install_all(radar_ip, username, password)
            else:
                print(f"[{time.strftime('%H:%M:%S')}] Tutti i {len(devices)} device OK", end='\r')

            time.sleep(interval)
        except KeyboardInterrupt:
            print("\n\nWatch mode terminato.")
            break
        except Exception as e:
            print(f"\n[ERRORE] {e}")
            time.sleep(interval)


def main():
    parser = argparse.ArgumentParser(description='Auto-installer Radar Service')
    parser.add_argument('--radar', default=RADAR_CLIENT_IP,
                        help=f'IP del RADAR_CLIENT ESP32 (default: {RADAR_CLIENT_IP})')
    parser.add_argument('--user', default=SSH_USERNAME,
                        help=f'Username SSH (default: {SSH_USERNAME})')
    parser.add_argument('--password', default=SSH_PASSWORD,
                        help=f'Password SSH (default: {SSH_PASSWORD})')
    parser.add_argument('--ip', default=None,
                        help='Installa su un device specifico (IP)')
    parser.add_argument('--watch', action='store_true',
                        help='Modalita daemon: controlla ogni 60 sec')
    parser.add_argument('--interval', type=int, default=60,
                        help='Intervallo watch mode in secondi (default: 60)')

    args = parser.parse_args()

    print("=" * 50)
    print("  RADAR SERVICE - AUTO INSTALLER")
    print("=" * 50)

    if args.watch:
        watch_mode(args.radar, args.user, args.password, args.interval)
    else:
        auto_install_all(args.radar, args.user, args.password, args.ip)


if __name__ == '__main__':
    main()
