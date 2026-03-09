"""Deploy radar_monitor_pi sul Raspberry Pi via SSH."""
import paramiko
import os
import time
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

PI_HOST = "192.168.1.122"
PI_USER = "pi"
PI_PASS = "RASPBERRY"
LOCAL_DIR = os.path.dirname(os.path.abspath(__file__))


def run(ssh, cmd, timeout=15):
    """Esegue comando via SSH."""
    stdin, stdout, stderr = ssh.exec_command(cmd, timeout=timeout)
    stdout.channel.recv_exit_status()
    out = stdout.read().decode("utf-8", errors="replace").strip()
    err = stderr.read().decode("utf-8", errors="replace").strip()
    return out, err


def main():
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    print(f"Connessione a {PI_USER}@{PI_HOST}...")
    ssh.connect(PI_HOST, username=PI_USER, password=PI_PASS, timeout=10)
    print("Connesso!\n")

    # Copia file
    sftp = ssh.open_sftp()
    local = os.path.join(LOCAL_DIR, "radar_monitor_pi.py")
    remote = f"/home/{PI_USER}/radar_monitor_pi.py"
    print(f"[COPIA] radar_monitor_pi.py -> Pi")
    sftp.put(local, remote)
    sftp.close()

    # Installa lo script
    print("[INSTALL] Copia in /usr/local/bin/...")
    run(ssh, f"sudo cp /home/{PI_USER}/radar_monitor_pi.py /usr/local/bin/")
    run(ssh, "sudo chmod +x /usr/local/bin/radar_monitor_pi.py")

    # Rimuovi il servizio systemd rotto
    print("[CLEANUP] Rimuovo service systemd (ciclico)...")
    run(ssh, "sudo systemctl stop radar_monitor_pi 2>/dev/null; true")
    run(ssh, "sudo systemctl disable radar_monitor_pi 2>/dev/null; true")
    run(ssh, "sudo rm -f /etc/systemd/system/radar_monitor_pi.service")
    run(ssh, "sudo systemctl daemon-reload")

    # Termina eventuale processo precedente
    print("[STOP] Termino processi precedenti...")
    run(ssh, "sudo pkill -f 'python3.*radar_monitor_pi' 2>/dev/null; true")
    time.sleep(1)

    # Avvio diretto con nohup
    print("[START] Avvio con nohup...")
    ssh.exec_command(
        "sudo nohup /usr/bin/python3 /usr/local/bin/radar_monitor_pi.py "
        "> /var/log/radar_monitor.log 2>&1 &"
    )
    time.sleep(3)

    # Verifica processo
    out, _ = run(ssh, "pgrep -f 'radar_monitor_pi' && echo 'RUNNING' || echo 'NOT RUNNING'")
    print(f"[STATUS] {out}")

    # Setup crontab per avvio al boot
    print("[BOOT] Configuro @reboot in crontab...")
    cron_line = "@reboot sudo /usr/bin/python3 /usr/local/bin/radar_monitor_pi.py > /var/log/radar_monitor.log 2>&1"
    # Rimuovi vecchia entry e aggiungi nuova
    run(ssh, f"(crontab -l 2>/dev/null | grep -v radar_monitor_pi; echo '{cron_line}') | crontab -")
    out, _ = run(ssh, "crontab -l 2>/dev/null | grep radar")
    print(f"    Crontab: {out}")

    # Test endpoint
    print("\n[TEST] Endpoint:")
    out, _ = run(ssh, "curl -s --connect-timeout 5 http://127.0.0.1/radar/ping")
    print(f"    /radar/ping = {out}")

    out, _ = run(ssh, "curl -s --connect-timeout 5 http://127.0.0.1/radar/on")
    print(f"    /radar/on   = {out}")

    out, _ = run(ssh, "curl -s --connect-timeout 5 http://127.0.0.1/radar/off")
    print(f"    /radar/off  = {out}")

    # Log
    out, _ = run(ssh, "cat /var/log/radar_monitor.log 2>/dev/null | tail -5")
    if out:
        print(f"\n[LOG]\n{out}")

    ssh.close()
    print("\n========================================")
    print("  INSTALLAZIONE COMPLETATA!")
    print(f"  http://{PI_HOST}/radar/on  = monitor ON")
    print(f"  http://{PI_HOST}/radar/off = monitor OFF")
    print("  Avvio automatico via crontab @reboot")
    print("========================================")


if __name__ == "__main__":
    main()
