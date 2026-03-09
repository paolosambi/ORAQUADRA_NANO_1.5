"""Run radar_monitor_pi directly to see errors."""
import paramiko
import time
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect("192.168.1.122", username="pi", password="RASPBERRY", timeout=10)

# Kill precedenti
ssh.exec_command("sudo pkill -f 'radar_monitor_pi' 2>/dev/null")
time.sleep(1)

# Esegui direttamente e cattura output/errori per 5 secondi
print("Avvio radar_monitor_pi.py direttamente...\n")
stdin, stdout, stderr = ssh.exec_command(
    "sudo /usr/bin/python3 /usr/local/bin/radar_monitor_pi.py 2>&1",
    timeout=10
)

# Leggi output per qualche secondo
stdout.channel.settimeout(8)
output = ""
try:
    while True:
        chunk = stdout.channel.recv(4096).decode("utf-8", errors="replace")
        if not chunk:
            break
        output += chunk
        print(chunk, end="", flush=True)
except Exception as e:
    print(f"\n[timeout/done: {e}]")

if not output:
    # Prova stderr
    try:
        stderr.channel.settimeout(3)
        err = stderr.channel.recv(4096).decode("utf-8", errors="replace")
        if err:
            print(f"\n[STDERR]\n{err}")
    except:
        pass

# Verifica se il processo gira dopo l'avvio
stdin2, stdout2, stderr2 = ssh.exec_command("pgrep -a -f radar_monitor_pi", timeout=5)
stdout2.channel.recv_exit_status()
out2 = stdout2.read().decode().strip()
print(f"\n[PROCESSI] {out2}")

ssh.close()
