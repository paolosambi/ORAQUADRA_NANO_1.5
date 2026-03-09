@echo off
title OraQuadra News Listener
echo ============================================
echo   OraQuadra Nano - News Listener
echo   Tocca una notizia sul display per aprirla
echo   Premi CTRL+C per chiudere
echo ============================================
echo.

powershell -ExecutionPolicy Bypass -Command ^
  "$host.UI.RawUI.WindowTitle='OraQuadra News Listener'; " ^
  "$ip = 'ORAQUADRA.local'; " ^
  "Write-Host 'Connessione a http://${ip}:8080 ...' -ForegroundColor Cyan; " ^
  "while($true) { " ^
  "  try { " ^
  "    $r = Invoke-RestMethod -Uri \"http://${ip}:8080/news/openarticle\" -TimeoutSec 3; " ^
  "    if ($r.url -and $r.url.Length -gt 0) { " ^
  "      Write-Host \"[$(Get-Date -Format 'HH:mm:ss')] Apro: $($r.url)\" -ForegroundColor Green; " ^
  "      Start-Process $r.url; " ^
  "    } " ^
  "  } catch { " ^
  "    Write-Host '.' -NoNewline -ForegroundColor DarkGray; " ^
  "  } " ^
  "  Start-Sleep -Seconds 2; " ^
  "}"
