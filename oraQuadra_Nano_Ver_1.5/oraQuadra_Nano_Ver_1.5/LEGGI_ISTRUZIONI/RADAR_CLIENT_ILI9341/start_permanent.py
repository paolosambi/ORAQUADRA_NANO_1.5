"""Start radar_monitor_pi permanently on Raspberry Pi."""
import paramiko
import time
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect("192.168.1.122", username="pi", password="RASPBERRY", timeout=10)
print("Connesso!\n")

def run(cmd, timeout=15):
    stdin, stdout, stderr = ssh.exec_command(cmd, timeout=timeout)
    stdout.channel.recv_exit_status()
    out = stdout.read().decode("utf-8", errors="replace").strip()
    err = stderr.read().decode("utf-8", errors="replace").strip()
    return out, err

# Kill precedenti
print("[STOP] Termino processi precedenti...")
run("sudo pkill -f 'radar_monitor_pi' 2>/dev/null; true")
time.sleep(1)

# Metodo: crea un piccolo script wrapper che fa il fork in C-style via bash
print("[SETUP] Creo start script sul Pi...")
start_script = r"""#!/bin/bash
# Kill precedenti
pkill -f 'radar_monitor_pi.py' 2>/dev/null
sleep 1
# Start come daemon vero (doppio fork)
(
    (
        exec /usr/bin/python3 -u /usr/local/bin/radar_monitor_pi.py \
            >> /var/log/radar_monitor.log 2>&1
    ) &
)
echo "Started with PID: $(pgrep -f 'radar_monitor_pi.py')"
"""

# Scrivi script sul Pi
sftp = ssh.open_sftp()
with sftp.open("/home/pi/start_radar.sh", "w") as f:
    f.write(start_script)
sftp.close()
run("chmod +x /home/pi/start_radar.sh")

# Esegui come root
print("[START] Esecuzione start script...")
out, err = run("sudo bash /home/pi/start_radar.sh", timeout=10)
print(f"    {out}")
if err:
    print(f"    [err] {err}")

time.sleep(3)

# Verifica
out, _ = run("pgrep -a -f 'python3.*radar_monitor_pi'")
print(f"\n[PROCESSI] {out or 'NESSUNO!'}")

out, _ = run("cat /var/log/radar_monitor.log 2>/dev/null | tail -10")
print(f"[LOG] {out or '(vuoto)'}")

# Se il processo non gira, prova con at
if not out or "radar" not in str(out):
    print("\n[ALTERNATIVA] Provo con 'at now'...")
    # Verifica se 'at' e' disponibile
    at_out, at_err = run("which at 2>/dev/null")
    screen_out, _ = run("which screen 2>/dev/null")
    tmux_out, _ = run("which tmux 2>/dev/null")
    print(f"    at: {at_out or 'NON DISPONIBILE'}")
    print(f"    screen: {screen_out or 'NON DISPONIBILE'}")
    print(f"    tmux: {tmux_out or 'NON DISPONIBILE'}")

    if screen_out:
        print("\n[START] Uso screen...")
        run("sudo screen -dmS radar /usr/bin/python3 -u /usr/local/bin/radar_monitor_pi.py")
        time.sleep(3)
        out, _ = run("screen -ls")
        print(f"    {out}")
    elif tmux_out:
        print("\n[START] Uso tmux...")
        run("sudo tmux new-session -d -s radar '/usr/bin/python3 -u /usr/local/bin/radar_monitor_pi.py'")
        time.sleep(3)

out, _ = run("pgrep -a -f 'python3.*radar_monitor_pi'")
print(f"\n[VERIFICA FINALE] {out or 'NESSUNO'}")

out, _ = run("curl -s --connect-timeout 5 http://127.0.0.1/radar/ping")
print(f"[TEST] /radar/ping = {out or 'NESSUNA RISPOSTA'}")

# Crontab root per boot
cron = "@reboot sleep 15 && /usr/bin/python3 -u /usr/local/bin/radar_monitor_pi.py >> /var/log/radar_monitor.log 2>&1"
run(f"(sudo crontab -l 2>/dev/null | grep -v radar_monitor_pi; echo '{cron}') | sudo crontab -")
out, _ = run("sudo crontab -l 2>/dev/null | grep radar")
print(f"[BOOT] {out}")

ssh.close()
print("\nDone!")
