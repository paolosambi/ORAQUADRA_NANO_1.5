"""Debug systemd issue on Raspberry Pi."""
import paramiko
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect("192.168.1.122", username="pi", password="RASPBERRY", timeout=10)

cmds = [
    "cat /etc/systemd/system/radar_monitor_pi.service",
    "sudo systemctl status radar_monitor_pi --no-pager 2>&1",
    "sudo journalctl -xe --no-pager -n 20 2>&1",
    "sudo systemctl list-dependencies radar_monitor_pi 2>&1",
    "sudo systemctl reset-failed radar_monitor_pi 2>&1; echo 'reset done'",
    "sudo systemctl daemon-reexec 2>&1; echo 'reexec done'",
    "sudo systemctl daemon-reload 2>&1; echo 'reload done'",
    "sudo systemctl start radar_monitor_pi 2>&1",
    "sudo systemctl status radar_monitor_pi --no-pager 2>&1",
    # Se systemd non funziona, prova avvio diretto
    "sudo python3 /usr/local/bin/radar_monitor_pi.py &",
    "sleep 2",
    "curl -s --connect-timeout 3 http://127.0.0.1/radar/ping 2>&1",
]

for cmd in cmds:
    print(f"\n>>> {cmd}")
    stdin, stdout, stderr = ssh.exec_command(cmd, timeout=20)
    stdout.channel.recv_exit_status()
    out = stdout.read().decode("utf-8", errors="replace").strip()
    err = stderr.read().decode("utf-8", errors="replace").strip()
    if out:
        for line in out.splitlines()[:20]:
            try:
                print(f"    {line}")
            except:
                print(f"    {line.encode('ascii','replace').decode()}")
    if err:
        for line in err.splitlines()[:10]:
            try:
                print(f"    [E] {line}")
            except:
                pass

ssh.close()
