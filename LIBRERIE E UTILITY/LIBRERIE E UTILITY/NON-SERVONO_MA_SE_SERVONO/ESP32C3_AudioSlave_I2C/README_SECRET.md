# Configurazione Credenziali WiFi

## Setup Iniziale

1. **Copia il file template:**
   ```
   secret.h.example → secret.h
   ```

2. **Modifica `secret.h` con le tue credenziali:**
   ```cpp
   // Access Point (obbligatorio per ESP32S3)
   #define WIFI_AP_SSID      "OraQuadra_Audio"
   #define WIFI_AP_PASSWORD  "oraquadra2025"

   // Rete locale (opzionale)
   #define WIFI_STA_SSID     "TuaReteWiFi"      // ← Inserisci qui
   #define WIFI_STA_PASSWORD "TuaPassword"      // ← Inserisci qui
   ```

3. **Compila e carica il codice sull'ESP32C3**

## Modalità di Funzionamento

### Solo Access Point (predefinito)
Se `WIFI_STA_SSID` è vuoto, l'ESP32C3 funziona solo come Access Point:
- IP: 192.168.4.1
- L'ESP32S3 si connette a questo IP
- OTA disponibile solo tramite l'AP

### Access Point + Rete Locale (modalità AP+STA)
Se configuri `WIFI_STA_SSID` e `WIFI_STA_PASSWORD`:
- **IP AP**: 192.168.4.1 (per ESP32S3)
- **IP Locale**: 192.168.x.x (dalla tua rete)
- L'ESP32S3 continua a usare 192.168.4.1
- OTA disponibile da entrambe le reti

## Sicurezza

⚠️ **IMPORTANTE:**
- Il file `secret.h` contiene le tue password
- NON condividere questo file
- NON caricarlo su repository pubblici (GitHub, GitLab, etc.)
- Il file `.gitignore` è configurato per escludere `secret.h`

## Ripristino

Se perdi il file `secret.h`:
1. Copia nuovamente `secret.h.example` come `secret.h`
2. Reinserisci le tue credenziali
