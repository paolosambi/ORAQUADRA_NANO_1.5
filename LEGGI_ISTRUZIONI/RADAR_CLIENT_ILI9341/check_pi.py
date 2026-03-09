"""Check what's happening on port 80 of the Pi."""
import paramiko
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect("192.168.1.122", username="pi", password="RASPBERRY", timeout=10)

cmds = [
    ("Log script", "cat /var/log/radar_monitor.log 2>/dev/null"),
    ("Processo attivo?", "ps aux | grep radar_monitor | grep -v grep"),
    ("Porta 80 in uso?", "sudo ss -tlnp | grep ':80 '"),
    ("Tutte le porte in ascolto", "sudo ss -tlnp | head -20"),
]

for desc, cmd in cmds:
    print(f"\n--- {desc} ---")
    stdin, stdout, stderr = ssh.exec_command(cmd, timeout=15)
    stdout.channel.recv_exit_status()
    out = stdout.read().decode("utf-8", errors="replace").strip()
    err = stderr.read().decode("utf-8", errors="replace").strip()
    if out:
        for line in out.splitlines():
            try:
                print(f"  {line}")
            except:
                print(f"  {line.encode('ascii','replace').decode()}")
    elif err:
        print(f"  [err] {err}")
    else:
        print("  (vuoto)")

ssh.close()
