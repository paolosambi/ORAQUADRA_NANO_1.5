#!/usr/bin/env python3
"""
============================================================================
MJPEG STREAMING SERVER per ESP32-4848S040
============================================================================

Server che scarica video da YouTube, li converte in MJPEG e li invia
in streaming all'ESP32 via WiFi.

REQUISITI:
  pip install flask yt-dlp opencv-python numpy

  Per audio streaming è necessario anche ffmpeg installato nel sistema:
  - Windows: scaricare da https://ffmpeg.org/download.html e aggiungere al PATH
  - Linux: sudo apt install ffmpeg
  - Mac: brew install ffmpeg

UTILIZZO:
  python stream_server.py

  Poi apri nel browser: http://localhost:5000
  L'ESP32 si connette a: http://<IP_PC>:5000/stream

============================================================================
"""

import os
import sys
import time
import threading
import subprocess
import tempfile
import shutil
from pathlib import Path
from queue import Queue, Empty
from io import BytesIO

try:
    from flask import Flask, Response, render_template_string, request, jsonify
    import cv2
    import numpy as np
except ImportError as e:
    print(f"Errore: {e}")
    print("\nInstalla le dipendenze con:")
    print("  pip install flask opencv-python numpy yt-dlp")
    sys.exit(1)

# ============================================================================
# CONFIGURAZIONE
# ============================================================================

SERVER_PORT = 5000
FRAME_WIDTH = 480
FRAME_HEIGHT = 480
TARGET_FPS = 25               # Aumentato da 15 a 25 FPS
JPEG_QUALITY = 50             # Ridotto da 70 a 50 (più veloce, file più piccoli)

# Percorso ffmpeg - modifica se ffmpeg non è nel PATH
FFMPEG_PATH = r"C:\Users\Paolo\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg.Essentials_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-8.0.1-essentials_build\bin\ffmpeg.exe"

# Directory temporanea per video scaricati
# Su Windows usa un percorso semplice per evitare problemi con OpenCV
if sys.platform == 'win32':
    # Usa una cartella nella directory dello script (evita problemi con percorsi lunghi)
    TEMP_DIR = Path(__file__).parent / "temp_videos"
else:
    TEMP_DIR = Path(tempfile.gettempdir()) / "mjpeg_server"
TEMP_DIR.mkdir(exist_ok=True)
print(f"[CONFIG] Directory video: {TEMP_DIR}")

# ============================================================================
# VARIABILI GLOBALI
# ============================================================================

app = Flask(__name__)

# Stato streaming con DOUBLE BUFFERING
class StreamState:
    def __init__(self):
        self.is_streaming = False
        self.is_downloading = False
        self.current_video = None
        self.video_path = None
        self.video_capture = None
        self.frame_queue = Queue(maxsize=30)
        self.current_frame = None
        self.frame_count = 0
        self.total_frames = 0
        self.fps = TARGET_FPS
        self.status = "Pronto"
        self.error = None
        self.clients = 0
        self.loop = True
        # Audio streaming
        self.audio_streaming = False
        self.audio_process = None
        self.audio_clients = 0
        self.audio_muted = False  # Stato mute controllato da ESP32
        self.audio_output = "browser"  # "browser" o "esp32c3" - scelta esclusiva
        self.video_looped = False  # Flag che indica se il video e' appena ricominciato (per sync audio C3)
        self.c3_volume = 80  # Volume ESP32-C3 (0-100)
        self.c3_volume_changed = False  # Flag per segnalare cambio volume a ESP32-S3
        self.c3_audio_delay = 0.0  # Delay audio C3 in secondi (-5 a +5, negativo=audio in ritardo)
        # DOUBLE BUFFERING - due buffer per frame alternati
        self.frame_buffer_a = None  # Buffer A
        self.frame_buffer_b = None  # Buffer B
        self.active_buffer = 'a'    # Buffer attualmente in uso per lettura
        self.buffer_lock = threading.Lock()  # Lock per accesso thread-safe

state = StreamState()
stream_lock = threading.Lock()
audio_lock = threading.Lock()

# ============================================================================
# FUNZIONI YOUTUBE
# ============================================================================

def check_ytdlp():
    """Verifica se yt-dlp è installato"""
    try:
        result = subprocess.run(['yt-dlp', '--version'],
                                capture_output=True, text=True)
        return result.returncode == 0
    except FileNotFoundError:
        return False

def download_youtube(url, progress_callback=None):
    """Scarica video da YouTube usando yt-dlp"""
    state.is_downloading = True
    state.status = "Download in corso..."
    state.error = None

    try:
        # Genera nome file unico - salva il timestamp
        timestamp = int(time.time())
        output_template = TEMP_DIR / f"video_{timestamp}.%(ext)s"

        # Trova ffmpeg nel PATH
        ffmpeg_path = shutil.which('ffmpeg')
        ffmpeg_dir = os.path.dirname(ffmpeg_path) if ffmpeg_path else None

        # Comando yt-dlp - usa formato 18 (360p non frammentato) per evitare blocchi
        # Il formato 18 è un formato progressivo (non DASH) che non si blocca
        cmd = [
            'yt-dlp',
            '-f', '18/22/best[ext=mp4]',  # 18=360p, 22=720p (formati progressivi), fallback mp4
            '--no-playlist',
            '-o', str(output_template),
            '--progress',
            '--newline',  # Output progress su righe separate
            '--extractor-args', 'youtube:player_client=android',
        ]

        # Aggiungi percorso ffmpeg se trovato
        if ffmpeg_dir:
            cmd.extend(['--ffmpeg-location', ffmpeg_dir])

        cmd.append(url)

        print(f"[DOWNLOAD] Comando: {' '.join(cmd)}", flush=True)

        # Usa Popen con PIPE e leggi output riga per riga
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1,
            encoding='utf-8',
            errors='replace'
        )

        # Leggi output riga per riga (non bloccante per il buffer)
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
            if line:
                line = line.strip()
                print(f"[YT-DLP] {line}", flush=True)
                if '[download]' in line and '%' in line:
                    try:
                        percent = line.split('%')[0].split()[-1]
                        state.status = f"Download: {percent}%"
                    except:
                        pass

        returncode = process.wait()

        if returncode != 0:
            raise Exception(f"yt-dlp errore (code {returncode})")

        # Cerca il file scaricato con qualsiasi estensione
        output_path = None
        for f in TEMP_DIR.glob(f"video_{timestamp}.*"):
            if f.suffix in ['.mp4', '.mkv', '.webm', '.avi', '.mov']:
                output_path = f
                break

        if output_path is None or not output_path.exists():
            raise Exception("File video non trovato dopo il download")

        state.video_path = str(output_path)
        state.status = "Download completato"
        print(f"[DOWNLOAD] Completato: {output_path}", flush=True)
        return True

    except Exception as e:
        state.error = str(e)
        state.status = f"Errore: {e}"
        print(f"[DOWNLOAD] Errore: {e}", flush=True)
        return False
    finally:
        state.is_downloading = False

# ============================================================================
# FUNZIONI STREAMING
# ============================================================================

def open_video(video_path):
    """Apre un video con OpenCV"""
    try:
        # Su Windows, OpenCV richiede percorsi con forward slash o raw string
        # Normalizza il percorso per evitare errori
        video_path = os.path.normpath(video_path)

        # Verifica che il file esista
        if not os.path.exists(video_path):
            raise Exception(f"File non trovato: {video_path}")

        print(f"[VIDEO] Tentativo apertura: {video_path}")

        # Su Windows, prova con CAP_FFMPEG se il percorso ha caratteri speciali
        cap = cv2.VideoCapture(video_path, cv2.CAP_FFMPEG)

        if not cap.isOpened():
            # Fallback: prova senza specificare backend
            print("[VIDEO] Fallback a backend automatico...")
            cap = cv2.VideoCapture(video_path)

        if not cap.isOpened():
            raise Exception("Impossibile aprire il video (OpenCV)")

        state.video_capture = cap
        state.total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        state.fps = cap.get(cv2.CAP_PROP_FPS) or TARGET_FPS
        state.frame_count = 0

        print(f"[VIDEO] Aperto: {video_path}")
        print(f"[VIDEO] Frame totali: {state.total_frames}")
        print(f"[VIDEO] FPS originale: {state.fps}")

        return True
    except Exception as e:
        state.error = str(e)
        print(f"[VIDEO] Errore apertura: {e}")
        import traceback
        traceback.print_exc()
        return False

def process_frame(frame):
    """Ridimensiona e converte un frame in JPEG"""
    # Ridimensiona a 480x480 mantenendo aspect ratio
    h, w = frame.shape[:2]

    # Calcola scala
    scale = min(FRAME_WIDTH / w, FRAME_HEIGHT / h)
    new_w = int(w * scale)
    new_h = int(h * scale)

    # Ridimensiona
    resized = cv2.resize(frame, (new_w, new_h), interpolation=cv2.INTER_LINEAR)

    # Crea immagine 480x480 con padding nero
    output = np.zeros((FRAME_HEIGHT, FRAME_WIDTH, 3), dtype=np.uint8)

    # Centra l'immagine
    x_offset = (FRAME_WIDTH - new_w) // 2
    y_offset = (FRAME_HEIGHT - new_h) // 2
    output[y_offset:y_offset+new_h, x_offset:x_offset+new_w] = resized

    # Codifica in JPEG
    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), JPEG_QUALITY]
    _, jpeg = cv2.imencode('.jpg', output, encode_param)

    return jpeg.tobytes()

def stream_thread():
    """Thread che genera i frame per lo streaming - CON DOUBLE BUFFERING"""
    frame_interval = 1.0 / TARGET_FPS

    while state.is_streaming:
        if state.video_capture is None:
            time.sleep(0.1)
            continue

        # Pausa video quando audio è mutato (sincronizzazione con browser)
        if state.audio_muted:
            time.sleep(0.1)
            continue

        start_time = time.time()

        ret, frame = state.video_capture.read()

        if not ret:
            if state.loop:
                # Ricomincia dal primo frame
                state.video_capture.set(cv2.CAP_PROP_POS_FRAMES, 0)
                state.frame_count = 0
                state.video_looped = True  # Segnala all'ESP32 che il video e' ricominciato
                print("[VIDEO] Loop - ricominciato dall'inizio")
                continue
            else:
                state.is_streaming = False
                state.status = "Video terminato"
                break

        state.frame_count += 1

        # Processa frame con DOUBLE BUFFERING
        try:
            jpeg_data = process_frame(frame)

            # DOUBLE BUFFERING: scrivi nel buffer inattivo, poi swap
            with state.buffer_lock:
                if state.active_buffer == 'a':
                    # Buffer A è attivo per lettura, scrivi in B
                    state.frame_buffer_b = jpeg_data
                    state.active_buffer = 'b'  # Swap: ora B è attivo
                else:
                    # Buffer B è attivo per lettura, scrivi in A
                    state.frame_buffer_a = jpeg_data
                    state.active_buffer = 'a'  # Swap: ora A è attivo

            # Mantieni anche current_frame per compatibilità
            state.current_frame = jpeg_data

            # Aggiungi alla coda (non bloccante)
            try:
                state.frame_queue.put_nowait(jpeg_data)
            except:
                pass  # Coda piena, salta frame

        except Exception as e:
            print(f"[STREAM] Errore frame: {e}")

        # Mantieni FPS target
        elapsed = time.time() - start_time
        sleep_time = frame_interval - elapsed
        if sleep_time > 0:
            time.sleep(sleep_time)

    print("[STREAM] Thread terminato")

def generate_mjpeg():
    """Generator per MJPEG stream - CON DOUBLE BUFFERING per flicker-free"""
    state.clients += 1
    print(f"[STREAM] Nuovo client connesso (totale: {state.clients})")

    last_sent_frame = None
    pause_send_interval = 0.5  # Invia frame ogni 500ms quando in pausa
    last_buffer_id = None  # Per tracciare quale buffer abbiamo già letto

    try:
        while state.is_streaming:
            # Quando in pausa, continua a inviare l'ultimo frame periodicamente
            # per mantenere la connessione attiva e il display stabile
            if state.audio_muted:
                if last_sent_frame:
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n'
                           b'Content-Length: ' + str(len(last_sent_frame)).encode() + b'\r\n\r\n' + last_sent_frame + b'\r\n')
                time.sleep(pause_send_interval)
                continue

            # DOUBLE BUFFERING: leggi dal buffer attivo
            frame = None
            current_buffer_id = None
            with state.buffer_lock:
                current_buffer_id = state.active_buffer
                if current_buffer_id == 'a' and state.frame_buffer_a is not None:
                    frame = state.frame_buffer_a
                elif current_buffer_id == 'b' and state.frame_buffer_b is not None:
                    frame = state.frame_buffer_b

            # Invia solo se c'è un nuovo buffer (evita di inviare lo stesso frame)
            if frame and current_buffer_id != last_buffer_id:
                last_buffer_id = current_buffer_id
                last_sent_frame = frame
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n'
                       b'Content-Length: ' + str(len(frame)).encode() + b'\r\n\r\n' + frame + b'\r\n')

            # Sleep ridotto per maggiore reattività
            time.sleep(0.005)  # 5ms invece di 40ms
    finally:
        state.clients -= 1
        print(f"[STREAM] Client disconnesso (totale: {state.clients})")

# ============================================================================
# FUNZIONI AUDIO STREAMING
# ============================================================================

def check_ffmpeg():
    """Verifica se ffmpeg è installato"""
    global FFMPEG_PATH

    # Prima prova il percorso configurato
    try:
        result = subprocess.run([FFMPEG_PATH, '-version'],
                                capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == 'win32' else 0)
        if result.returncode == 0:
            return True
    except FileNotFoundError:
        pass

    # Cerca in percorsi comuni Windows
    common_paths = [
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        os.path.expanduser("~/ffmpeg/bin/ffmpeg.exe"),
        os.path.expanduser("~/Downloads/ffmpeg-8.0.1-essentials_build/bin/ffmpeg.exe"),
    ]

    for path in common_paths:
        if os.path.exists(path):
            FFMPEG_PATH = path
            print(f"[AUDIO] ffmpeg trovato in: {path}")
            return True

    # Usa shutil.which come ultima risorsa
    found = shutil.which('ffmpeg')
    if found:
        FFMPEG_PATH = found
        print(f"[AUDIO] ffmpeg trovato via PATH: {found}")
        return True

    return False

def start_audio_extraction(seek_time=None, delay_offset=3.0):
    """Avvia estrazione audio con ffmpeg, sincronizzato con il video

    Args:
        seek_time: Posizione di partenza in secondi (None = calcola da frame corrente)
        delay_offset: Ritardo audio rispetto al video per compensare buffering MJPEG
                     (l'audio viene avviato delay_offset secondi DOPO la posizione video)
    """
    if not state.video_path:
        print("[AUDIO] Nessun video caricato")
        return False

    if not check_ffmpeg():
        print("[AUDIO] ffmpeg non trovato!")
        return False

    # Calcola la posizione temporale corrente basata sul frame_count
    if seek_time is None and state.fps > 0 and state.frame_count > 0:
        seek_time = state.frame_count / state.fps

    # Per ESP32-C3, applica delay configurabile per sincronizzazione
    # - Delay positivo: audio in ANTICIPO (salta avanti nel video)
    # - Delay negativo: audio in RITARDO (parte da posizione precedente)
    if state.audio_output == "esp32c3":
        # Parti dall'inizio + delay configurato
        audio_start = max(0, state.c3_audio_delay)  # Non andare sotto 0
        print(f"[AUDIO] ESP32-C3: partenza da {audio_start:.1f}s (delay={state.c3_audio_delay:.1f}s)")
        seek_time = audio_start

    # Comando ffmpeg: estrai audio e converti in MP3 streaming
    # Bitrate basso per ESP32 con memoria limitata
    cmd = [FFMPEG_PATH, '-y']

    # Aggiungi seek PRIMA dell'input per sincronizzazione (più efficiente)
    if seek_time and seek_time > 0.5:  # Solo se > 0.5 secondi
        cmd.extend(['-ss', f'{seek_time:.2f}'])
        print(f"[AUDIO] Sincronizzazione a {seek_time:.2f} secondi")

    cmd.extend([
        '-i', state.video_path,   # Input video
        '-vn',                    # No video
        '-acodec', 'libmp3lame',  # Codec MP3
        '-ab', '96k',             # Bitrate 96kbps (ridotto per ESP32)
        '-ar', '22050',           # Sample rate 22kHz (ridotto per ESP32)
        '-ac', '1',               # Mono (risparmia banda e memoria)
        '-f', 'mp3',              # Format MP3
        'pipe:1'                  # Output a stdout (pipe esplicito)
    ])

    print(f"[AUDIO] Avvio ffmpeg: {' '.join(cmd)}")

    try:
        # IMPORTANTE: creationflags per Windows per evitare finestra console
        kwargs = {}
        if sys.platform == 'win32':
            kwargs['creationflags'] = subprocess.CREATE_NO_WINDOW

        state.audio_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,  # Cattura stderr per debug
            bufsize=32768,  # Buffer più grande per streaming fluido
            **kwargs
        )
        state.audio_streaming = True
        print("[AUDIO] Streaming audio avviato")
        return True
    except Exception as e:
        print(f"[AUDIO] Errore avvio ffmpeg: {e}")
        return False

def stop_audio_extraction():
    """Ferma estrazione audio"""
    state.audio_streaming = False
    if state.audio_process:
        try:
            state.audio_process.terminate()
            state.audio_process.wait(timeout=2)
        except:
            state.audio_process.kill()
        state.audio_process = None
    print("[AUDIO] Streaming audio fermato")

def generate_audio_stream():
    """Generator per audio MP3 stream con rate limiting per sincronizzazione"""
    with audio_lock:
        state.audio_clients += 1
    print(f"[AUDIO] Client connesso (totale: {state.audio_clients})")

    bytes_sent = 0
    start_time = time.time()

    # Rate limiting: 96kbps = 12000 bytes/secondo (96000 bits / 8)
    # Aggiungiamo un po' di margine per non rallentare troppo
    target_bytes_per_second = 12000 * 1.1  # 10% margine

    try:
        # Se non c'è già un processo ffmpeg, avvialo
        if not state.audio_process or state.audio_process.poll() is not None:
            if not start_audio_extraction():
                print("[AUDIO] Impossibile avviare ffmpeg")
                yield b''
                return

        # Leggi e invia chunk audio con rate limiting
        chunk_size = 2048  # Chunk leggermente più grandi
        while state.audio_streaming and state.audio_process:
            try:
                # Verifica che il processo sia ancora in esecuzione
                if state.audio_process.poll() is not None:
                    print(f"[AUDIO] ffmpeg terminato (returncode: {state.audio_process.returncode})")
                    # Leggi eventuale errore
                    stderr = state.audio_process.stderr.read()
                    if stderr:
                        print(f"[AUDIO] ffmpeg stderr: {stderr.decode('utf-8', errors='ignore')[:500]}")

                    if state.loop and state.video_path:
                        print("[AUDIO] Riavvio per loop...")
                        stop_audio_extraction()
                        time.sleep(0.2)
                        if not start_audio_extraction(seek_time=0):  # Riparti dall'inizio
                            break
                        # Reset timing per il loop
                        start_time = time.time()
                        bytes_sent = 0
                        continue
                    else:
                        break

                chunk = state.audio_process.stdout.read(chunk_size)
                if chunk:
                    bytes_sent += len(chunk)
                    yield chunk

                    # Rate limiting: calcola quanto tempo dovrebbe essere passato
                    elapsed = time.time() - start_time
                    expected_bytes = elapsed * target_bytes_per_second

                    # Se siamo troppo avanti, rallenta
                    if bytes_sent > expected_bytes + 5000:  # 5KB di tolleranza
                        sleep_time = (bytes_sent - expected_bytes) / target_bytes_per_second
                        if sleep_time > 0.01:  # Solo se > 10ms
                            time.sleep(min(sleep_time, 0.1))  # Max 100ms di sleep
                else:
                    # Nessun dato disponibile, breve pausa
                    time.sleep(0.01)

            except Exception as e:
                print(f"[AUDIO] Errore lettura: {e}")
                break

        print(f"[AUDIO] Totale bytes inviati: {bytes_sent}")

    finally:
        with audio_lock:
            state.audio_clients -= 1
        print(f"[AUDIO] Client disconnesso (totale: {state.audio_clients})")

        # Se non ci sono più client, ferma ffmpeg
        if state.audio_clients <= 0:
            stop_audio_extraction()

# ============================================================================
# ROUTES FLASK
# ============================================================================

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>MJPEG Streaming Server</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #fff;
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 900px; margin: 0 auto; }
        h1 {
            text-align: center;
            margin-bottom: 20px;
            color: #00d4ff;
            text-shadow: 0 0 10px rgba(0,212,255,0.5);
        }
        .subtitle { text-align: center; color: #888; margin-bottom: 30px; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
        }
        .card h2 { color: #00d4ff; margin-bottom: 15px; font-size: 1.2em; }
        input[type="text"] {
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 10px;
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 16px;
            margin-bottom: 10px;
        }
        input[type="text"]::placeholder { color: #666; }
        button {
            padding: 12px 25px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            font-size: 16px;
            font-weight: bold;
            transition: all 0.3s;
            margin: 5px;
        }
        .btn-primary {
            background: linear-gradient(135deg, #00d4ff, #0099cc);
            color: #fff;
        }
        .btn-primary:hover { transform: scale(1.05); box-shadow: 0 5px 20px rgba(0,212,255,0.4); }
        .btn-danger { background: #ff4757; color: #fff; }
        .btn-danger:hover { background: #ff6b7a; }
        .btn-success { background: #2ed573; color: #fff; }
        .btn-success:hover { background: #7bed9f; }
        .status-bar {
            display: flex;
            justify-content: space-between;
            flex-wrap: wrap;
            gap: 10px;
        }
        .status-item {
            background: rgba(0,0,0,0.3);
            padding: 10px 15px;
            border-radius: 8px;
            flex: 1;
            min-width: 120px;
            text-align: center;
        }
        .status-label { color: #888; font-size: 12px; }
        .status-value { color: #00d4ff; font-size: 18px; font-weight: bold; }
        .preview {
            text-align: center;
            margin-top: 20px;
        }
        .preview img {
            max-width: 100%;
            border-radius: 10px;
            border: 2px solid #00d4ff;
        }
        .info-box {
            background: rgba(0,212,255,0.1);
            border-left: 4px solid #00d4ff;
            padding: 15px;
            margin-top: 20px;
            border-radius: 0 10px 10px 0;
        }
        .info-box code {
            background: rgba(0,0,0,0.3);
            padding: 3px 8px;
            border-radius: 5px;
            color: #2ed573;
        }
        .controls { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 15px; }
        #log {
            background: #000;
            color: #0f0;
            padding: 15px;
            border-radius: 10px;
            height: 150px;
            overflow-y: auto;
            font-family: monospace;
            font-size: 12px;
            margin-top: 15px;
        }
        .streaming { animation: pulse 2s infinite; }
        @keyframes pulse {
            0%, 100% { box-shadow: 0 0 0 0 rgba(0,212,255,0.4); }
            50% { box-shadow: 0 0 0 15px rgba(0,212,255,0); }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>MJPEG Streaming Server</h1>
        <p class="subtitle">Stream video YouTube su ESP32-4848S040</p>

        <div class="card">
            <h2>1. Inserisci URL YouTube</h2>
            <input type="text" id="url" placeholder="https://www.youtube.com/watch?v=...">
            <div class="controls">
                <button class="btn-primary" onclick="downloadVideo()">Download</button>
                <button class="btn-success" onclick="loadLocal()">Carica File Locale</button>
            </div>
        </div>

        <div class="card">
            <h2>2. Stato</h2>
            <div class="status-bar">
                <div class="status-item">
                    <div class="status-label">Stato</div>
                    <div class="status-value" id="status">Pronto</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Frame</div>
                    <div class="status-value" id="frames">0 / 0</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Client</div>
                    <div class="status-value" id="clients">0</div>
                </div>
                <div class="status-item">
                    <div class="status-label">FPS</div>
                    <div class="status-value" id="fps">15</div>
                </div>
            </div>
            <div id="log"></div>
        </div>

        <div class="card" id="stream-card">
            <h2>3. Controlli Streaming</h2>
            <div class="controls">
                <button class="btn-success" onclick="startStream()">▶ Play</button>
                <button class="btn-danger" onclick="stopStream()">■ Stop</button>
                <label style="display:flex;align-items:center;color:#888;">
                    <input type="checkbox" id="loop" checked style="margin-right:8px;"> Loop
                </label>
                <label style="display:flex;align-items:center;color:#888;">
                    <input type="checkbox" id="audioEnabled" checked style="margin-right:8px;"> Audio
                </label>
                <div style="display:flex;align-items:center;gap:8px;margin-left:10px;">
                    <span style="color:#888;">Output:</span>
                    <select id="audioOutput" onchange="setAudioOutput()" style="padding:8px;border-radius:8px;background:#333;color:#fff;border:1px solid #00d4ff;">
                        <option value="browser">Browser</option>
                        <option value="esp32c3">ESP32-C3</option>
                    </select>
                </div>
            </div>

            <div class="preview" id="preview-container" style="display:none;">
                <h3 style="margin:15px 0;color:#888;">Anteprima Video + Audio (browser)</h3>
                <img id="preview" src="" alt="Stream preview">
                <div style="margin-top:15px;">
                    <audio id="audioPlayer" controls style="width:100%;"></audio>
                    <div id="browserVolumeControl" style="display:flex;align-items:center;gap:10px;margin-top:10px;">
                        <span style="color:#888;">🔊 Volume Browser:</span>
                        <input type="range" id="volumeSlider" min="0" max="100" value="80"
                               style="flex:1;accent-color:#00d4ff;">
                        <span id="volumeValue" style="color:#00d4ff;min-width:40px;">80%</span>
                    </div>
                    <div id="c3VolumeControl" style="display:none;align-items:center;gap:10px;margin-top:10px;">
                        <span style="color:#888;">🔊 Volume ESP32-C3:</span>
                        <input type="range" id="c3VolumeSlider" min="0" max="95" value="80"
                               style="flex:1;accent-color:#2ed573;">
                        <span id="c3VolumeValue" style="color:#2ed573;min-width:40px;">80%</span>
                    </div>
                    <div id="c3DelayControl" style="display:none;align-items:center;gap:10px;margin-top:10px;">
                        <span style="color:#888;">⏱️ Delay Audio (sec):</span>
                        <input type="range" id="c3DelaySlider" min="-5" max="5" value="0" step="0.5"
                               style="flex:1;accent-color:#ffa502;">
                        <span id="c3DelayValue" style="color:#ffa502;min-width:50px;">0.0s</span>
                    </div>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>4. Connessione ESP32</h2>
            <div class="info-box">
                <p><strong>URL Stream per ESP32:</strong></p>
                <p><code id="stream-url">http://{{host}}:{{port}}/stream</code></p>
                <p style="margin-top:10px;color:#888;">
                    Inserisci questo URL nello sketch ESP32 per ricevere lo stream MJPEG.
                </p>
            </div>
        </div>

        <div class="card">
            <h2>5. Gestione Server</h2>
            <div class="controls">
                <button class="btn-danger" onclick="restartServer()" style="background:linear-gradient(135deg,#ff6b6b,#ee5a5a);">
                    ↻ Riavvia Server
                </button>
                <button class="btn-danger" onclick="clearCache()" style="background:linear-gradient(135deg,#ffa502,#ff7f00);">
                    🗑 Elimina Cache Video
                </button>
            </div>
            <p style="margin-top:10px;color:#888;font-size:0.9em;">
                Riavvia il server Python per applicare modifiche o risolvere problemi.
            </p>
            <p id="cacheInfo" style="margin-top:5px;color:#ffa502;font-size:0.85em;"></p>
        </div>
    </div>

    <script>
        function log(msg) {
            const logEl = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            logEl.innerHTML += `[${time}] ${msg}\\n`;
            logEl.scrollTop = logEl.scrollHeight;
        }

        function updateStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('status').textContent = data.status;
                    document.getElementById('frames').textContent =
                        `${data.frame_count} / ${data.total_frames}`;
                    document.getElementById('clients').textContent = data.clients;
                    document.getElementById('fps').textContent = Math.round(data.fps);

                    const card = document.getElementById('stream-card');
                    if (data.is_streaming) {
                        card.classList.add('streaming');
                    } else {
                        card.classList.remove('streaming');
                    }

                    // Controlla stato mute da ESP32
                    const audioPlayer = document.getElementById('audioPlayer');
                    if (audioPlayer) {
                        if (data.audio_muted && !audioPlayer.paused) {
                            audioPlayer.pause();
                            log('Audio mutato da ESP32');
                        } else if (!data.audio_muted && audioPlayer.paused && audioPlayer.src && data.is_streaming) {
                            audioPlayer.play().catch(() => {});
                            log('Audio riattivato da ESP32');
                        }
                    }
                });
        }

        function downloadVideo() {
            const url = document.getElementById('url').value;
            if (!url) {
                alert('Inserisci un URL YouTube!');
                return;
            }
            log('Avvio download: ' + url);
            fetch('/api/download', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({url: url})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    log('Download avviato');
                } else {
                    log('Errore: ' + data.error);
                }
            });
        }

        function loadLocal() {
            const path = prompt('Inserisci il percorso completo del file video:');
            if (path) {
                fetch('/api/load', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({path: path})
                })
                .then(r => r.json())
                .then(data => {
                    if (data.success) {
                        log('Video caricato: ' + path);
                    } else {
                        log('Errore: ' + data.error);
                    }
                });
            }
        }

        function startStream() {
            const loop = document.getElementById('loop').checked;
            const audioEnabled = document.getElementById('audioEnabled').checked;
            const audioOutput = document.getElementById('audioOutput').value;
            fetch('/api/start', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({loop: loop})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    log('Streaming video avviato');
                    document.getElementById('preview-container').style.display = 'block';
                    document.getElementById('preview').src = '/stream?' + Date.now();

                    // Avvia audio dal browser SOLO se output=browser
                    const audioPlayer = document.getElementById('audioPlayer');
                    if (audioEnabled && audioOutput === 'browser') {
                        audioPlayer.src = '/audio?' + Date.now();
                        audioPlayer.loop = loop;
                        audioPlayer.play().then(() => {
                            log('Audio browser avviato');
                        }).catch(err => {
                            log('Audio richiede interazione utente - premi play');
                        });
                    } else if (audioOutput === 'esp32c3') {
                        log('Audio su ESP32-C3 (non browser)');
                        audioPlayer.src = '';
                    }
                } else {
                    log('Errore: ' + data.error);
                }
            });
        }

        function stopStream() {
            // Ferma audio browser
            const audioPlayer = document.getElementById('audioPlayer');
            audioPlayer.pause();
            audioPlayer.src = '';

            fetch('/api/stop', {method: 'POST'})
                .then(r => r.json())
                .then(data => {
                    log('Streaming video e audio fermato');
                    document.getElementById('preview-container').style.display = 'none';
                });
        }

        // Controllo volume slider browser
        document.getElementById('volumeSlider').addEventListener('input', function() {
            const volume = this.value / 100;
            const audioPlayer = document.getElementById('audioPlayer');
            audioPlayer.volume = volume;
            document.getElementById('volumeValue').textContent = this.value + '%';
        });

        // Controllo volume slider ESP32-C3
        document.getElementById('c3VolumeSlider').addEventListener('input', function() {
            const volume = this.value;
            document.getElementById('c3VolumeValue').textContent = volume + '%';
            // Invia volume al server
            fetch('/api/volume/c3', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({volume: parseInt(volume)})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    log('Volume C3: ' + volume + '%');
                }
            });
        });

        // Controllo delay slider ESP32-C3
        document.getElementById('c3DelaySlider').addEventListener('input', function() {
            const delay = parseFloat(this.value);
            document.getElementById('c3DelayValue').textContent = delay.toFixed(1) + 's';
            // Invia delay al server
            fetch('/api/audio/delay', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({delay: delay})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    log('Delay audio C3: ' + delay.toFixed(1) + 's');
                }
            });
        });

        // Imposta volume iniziale
        document.getElementById('audioPlayer').volume = 0.8;

        // Funzione per mostrare/nascondere controlli volume in base a output
        function updateVolumeControls(output) {
            const browserCtrl = document.getElementById('browserVolumeControl');
            const c3Ctrl = document.getElementById('c3VolumeControl');
            const c3DelayCtrl = document.getElementById('c3DelayControl');
            if (output === 'esp32c3') {
                browserCtrl.style.display = 'none';
                c3Ctrl.style.display = 'flex';
                c3DelayCtrl.style.display = 'flex';
            } else {
                browserCtrl.style.display = 'flex';
                c3Ctrl.style.display = 'none';
                c3DelayCtrl.style.display = 'none';
            }
        }

        // Funzione per impostare output audio
        function setAudioOutput() {
            const output = document.getElementById('audioOutput').value;
            fetch('/api/audio/output', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({output: output})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    log('Audio output: ' + output);
                    // Aggiorna controlli volume visibili
                    updateVolumeControls(output);
                    // Se cambiato a ESP32-C3, ferma audio browser
                    const audioPlayer = document.getElementById('audioPlayer');
                    if (output === 'esp32c3') {
                        audioPlayer.pause();
                        audioPlayer.src = '';
                        log('Audio browser disabilitato');
                    }
                }
            });
        }

        // Carica impostazione audio output e volume C3 all'avvio
        function loadAudioOutput() {
            fetch('/api/audio/output')
                .then(r => r.json())
                .then(data => {
                    const output = data.output || 'browser';
                    document.getElementById('audioOutput').value = output;
                    updateVolumeControls(output);
                });
            // Carica anche volume C3
            fetch('/api/volume/c3')
                .then(r => r.json())
                .then(data => {
                    const vol = data.volume || 80;
                    document.getElementById('c3VolumeSlider').value = vol;
                    document.getElementById('c3VolumeValue').textContent = vol + '%';
                });
        }

        // Riavvia il server Python
        function restartServer() {
            if (!confirm('Vuoi riavviare il server Python?\\nLa pagina si ricaricherà automaticamente.')) {
                return;
            }
            log('Riavvio server in corso...');
            fetch('/api/restart', {method: 'POST'})
                .then(() => {
                    log('Server in riavvio, attendo...');
                    // Attendi che il server si riavvii e ricarica la pagina
                    setTimeout(() => {
                        location.reload();
                    }, 3000);
                })
                .catch(() => {
                    // Il server si è spento, prova a ricaricare dopo un po'
                    log('Server spento, ricarico tra 3 secondi...');
                    setTimeout(() => {
                        location.reload();
                    }, 3000);
                });
        }

        // Elimina cache video temporanei
        function clearCache() {
            if (!confirm('Vuoi eliminare tutti i video scaricati dalla cache?\\nI video dovranno essere riscaricati.')) {
                return;
            }
            log('Eliminazione cache video...');
            fetch('/api/clear_cache', {method: 'POST'})
                .then(r => r.json())
                .then(data => {
                    if (data.success) {
                        log('Cache eliminata: ' + data.message);
                        document.getElementById('cacheInfo').textContent = data.message;
                        document.getElementById('cacheInfo').style.color = '#2ed573';
                    } else {
                        log('Errore: ' + data.error);
                        document.getElementById('cacheInfo').textContent = 'Errore: ' + data.error;
                        document.getElementById('cacheInfo').style.color = '#ff4757';
                    }
                })
                .catch(err => {
                    log('Errore eliminazione cache: ' + err);
                    document.getElementById('cacheInfo').textContent = 'Errore connessione';
                    document.getElementById('cacheInfo').style.color = '#ff4757';
                });
        }

        // Aggiorna stato ogni secondo
        setInterval(updateStatus, 1000);
        updateStatus();
        loadAudioOutput();

        log('Server pronto');
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    """Pagina principale"""
    import socket
    host = socket.gethostbyname(socket.gethostname())
    return render_template_string(HTML_TEMPLATE, host=host, port=SERVER_PORT)

@app.route('/stream')
def stream():
    """Endpoint MJPEG stream"""
    if not state.is_streaming:
        return "Stream non attivo", 503

    return Response(
        generate_mjpeg(),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )

@app.route('/frame')
def single_frame():
    """Singolo frame JPEG"""
    if state.current_frame:
        return Response(state.current_frame, mimetype='image/jpeg')
    return "Nessun frame disponibile", 503

@app.route('/audio')
def audio():
    """Endpoint audio MP3 stream (compatibile ICY/Shoutcast per ESP32)"""
    if not state.is_streaming:
        return "Stream non attivo", 503

    if not state.video_path:
        return "Nessun video caricato", 503

    if not check_ffmpeg():
        return "ffmpeg non installato", 503

    # Header compatibili con AudioFileSourceICYStream di ESP8266Audio
    # Questi header sono essenziali per la libreria ESP32
    return Response(
        generate_audio_stream(),
        mimetype='audio/mpeg',
        headers={
            'Content-Type': 'audio/mpeg',
            'Cache-Control': 'no-cache, no-store',
            'Connection': 'close',
            'icy-br': '96',                    # Bitrate ICY
            'icy-name': 'MJPEG Audio Stream',  # Nome stream
            'icy-genre': 'Video Audio',        # Genere
            'icy-pub': '1',                    # Pubblico
            'icy-metaint': '0',                # Nessun metadata inline
        }
    )

# ============================================================================
# AUDIO PCM SINCRONIZZATO PER BLUETOOTH ESP32-S3
# ============================================================================

def start_audio_pcm_extraction(seek_time=None):
    """Avvia estrazione audio PCM (raw) per streaming Bluetooth"""
    if not state.video_path:
        print("[AUDIO-PCM] Nessun video caricato")
        return False

    if not check_ffmpeg():
        print("[AUDIO-PCM] ffmpeg non trovato!")
        return False

    # Calcola la posizione temporale corrente basata sul frame_count
    if seek_time is None and state.fps > 0 and state.frame_count > 0:
        seek_time = state.frame_count / state.fps

    # Comando ffmpeg: estrai audio come PCM signed 16-bit stereo
    cmd = [FFMPEG_PATH, '-y']

    # Aggiungi seek PRIMA dell'input per sincronizzazione
    if seek_time and seek_time > 0.5:
        cmd.extend(['-ss', f'{seek_time:.2f}'])
        print(f"[AUDIO-PCM] Sincronizzazione a {seek_time:.2f} secondi")

    cmd.extend([
        '-i', state.video_path,   # Input video
        '-vn',                    # No video
        '-acodec', 'pcm_s16le',   # PCM signed 16-bit little endian
        '-ar', '44100',           # Sample rate 44.1kHz (standard per A2DP)
        '-ac', '2',               # Stereo (richiesto per A2DP)
        '-f', 's16le',            # Format raw PCM
        'pipe:1'                  # Output a stdout
    ])

    print(f"[AUDIO-PCM] Avvio ffmpeg: {' '.join(cmd)}")

    try:
        kwargs = {}
        if sys.platform == 'win32':
            kwargs['creationflags'] = subprocess.CREATE_NO_WINDOW

        state.pcm_audio_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=65536,  # Buffer grande per streaming fluido
            **kwargs
        )
        state.pcm_audio_streaming = True
        print("[AUDIO-PCM] Streaming audio PCM avviato")
        return True
    except Exception as e:
        print(f"[AUDIO-PCM] Errore avvio ffmpeg: {e}")
        return False

def stop_audio_pcm_extraction():
    """Ferma estrazione audio PCM"""
    state.pcm_audio_streaming = False
    if hasattr(state, 'pcm_audio_process') and state.pcm_audio_process:
        try:
            state.pcm_audio_process.terminate()
            state.pcm_audio_process.wait(timeout=2)
        except:
            state.pcm_audio_process.kill()
        state.pcm_audio_process = None
    print("[AUDIO-PCM] Streaming audio PCM fermato")

def generate_audio_pcm_stream():
    """Generator per audio PCM stream sincronizzato con timestamp"""
    print(f"[AUDIO-PCM] Nuovo client connesso")

    bytes_sent = 0
    start_time = time.time()

    # PCM 44100Hz stereo 16-bit = 176400 bytes/secondo
    target_bytes_per_second = 44100 * 2 * 2  # sample_rate * channels * bytes_per_sample

    try:
        # Avvia processo ffmpeg se non attivo
        if not hasattr(state, 'pcm_audio_process') or state.pcm_audio_process is None or state.pcm_audio_process.poll() is not None:
            if not start_audio_pcm_extraction():
                print("[AUDIO-PCM] Impossibile avviare ffmpeg")
                yield b''
                return

        chunk_size = 4096  # 4KB chunks
        while state.pcm_audio_streaming and hasattr(state, 'pcm_audio_process') and state.pcm_audio_process:
            try:
                # Verifica processo ancora in esecuzione
                if state.pcm_audio_process.poll() is not None:
                    print(f"[AUDIO-PCM] ffmpeg terminato")
                    if state.loop and state.video_path:
                        print("[AUDIO-PCM] Riavvio per loop...")
                        stop_audio_pcm_extraction()
                        time.sleep(0.1)
                        if not start_audio_pcm_extraction(seek_time=0):
                            break
                        start_time = time.time()
                        bytes_sent = 0
                        continue
                    else:
                        break

                chunk = state.pcm_audio_process.stdout.read(chunk_size)
                if chunk:
                    # Calcola timestamp corrente in ms
                    current_timestamp = int((bytes_sent / target_bytes_per_second) * 1000)

                    # Invia header con timestamp (4 bytes) + dati audio
                    # Formato: [TIMESTAMP_MS (4 bytes big-endian)] [PCM_DATA]
                    timestamp_header = current_timestamp.to_bytes(4, 'big')
                    yield timestamp_header + chunk

                    bytes_sent += len(chunk)

                    # Rate limiting per sincronizzazione
                    elapsed = time.time() - start_time
                    expected_bytes = elapsed * target_bytes_per_second
                    if bytes_sent > expected_bytes + 10000:
                        sleep_time = (bytes_sent - expected_bytes) / target_bytes_per_second
                        if sleep_time > 0.01:
                            time.sleep(min(sleep_time, 0.1))
                else:
                    time.sleep(0.01)

            except Exception as e:
                print(f"[AUDIO-PCM] Errore lettura: {e}")
                break

        print(f"[AUDIO-PCM] Totale bytes inviati: {bytes_sent}")

    finally:
        print(f"[AUDIO-PCM] Client disconnesso")
        stop_audio_pcm_extraction()

@app.route('/audio_pcm')
def audio_pcm():
    """Endpoint audio PCM stream sincronizzato per Bluetooth ESP32-S3"""
    if not state.is_streaming:
        return "Stream non attivo", 503

    if not state.video_path:
        return "Nessun video caricato", 503

    if not check_ffmpeg():
        return "ffmpeg non installato", 503

    # Inizializza variabili se non esistono
    if not hasattr(state, 'pcm_audio_streaming'):
        state.pcm_audio_streaming = False
    if not hasattr(state, 'pcm_audio_process'):
        state.pcm_audio_process = None

    return Response(
        generate_audio_pcm_stream(),
        mimetype='audio/x-raw',
        headers={
            'Content-Type': 'audio/x-raw;rate=44100;channels=2;format=S16LE',
            'Cache-Control': 'no-cache, no-store',
            'Connection': 'close',
            'X-Audio-Sync': 'timestamp-header',  # Indica che ogni chunk ha header timestamp
        }
    )

@app.route('/api/sync_info')
def api_sync_info():
    """Restituisce info sincronizzazione audio/video"""
    video_timestamp = 0
    if state.fps > 0 and state.frame_count > 0:
        video_timestamp = int((state.frame_count / state.fps) * 1000)

    return jsonify({
        'video_timestamp_ms': video_timestamp,
        'frame_count': state.frame_count,
        'fps': state.fps,
        'is_streaming': state.is_streaming,
        'audio_muted': state.audio_muted
    })

@app.route('/api/status')
def api_status():
    """Stato corrente"""
    return jsonify({
        'is_streaming': state.is_streaming,
        'is_downloading': state.is_downloading,
        'status': state.status,
        'error': state.error,
        'frame_count': state.frame_count,
        'total_frames': state.total_frames,
        'fps': state.fps,
        'clients': state.clients,
        'video_path': state.video_path,
        'audio_streaming': state.audio_streaming,
        'audio_clients': state.audio_clients,
        'audio_muted': state.audio_muted,
        'audio_output': state.audio_output
    })

@app.route('/api/download', methods=['POST'])
def api_download():
    """Scarica video da YouTube"""
    if state.is_downloading:
        return jsonify({'success': False, 'error': 'Download già in corso'})

    if not check_ytdlp():
        return jsonify({'success': False,
                       'error': 'yt-dlp non installato. Esegui: pip install yt-dlp'})

    data = request.json
    url = data.get('url', '')

    if not url:
        return jsonify({'success': False, 'error': 'URL mancante'})

    # Avvia download in background
    thread = threading.Thread(target=download_youtube, args=(url,))
    thread.daemon = True
    thread.start()

    return jsonify({'success': True})

@app.route('/api/load', methods=['POST'])
def api_load():
    """Carica video locale"""
    data = request.json
    path = data.get('path', '')

    if not path or not os.path.exists(path):
        return jsonify({'success': False, 'error': 'File non trovato'})

    state.video_path = path
    state.status = "Video caricato"
    return jsonify({'success': True})

@app.route('/api/start', methods=['POST'])
def api_start():
    """Avvia streaming video e audio sincronizzati"""
    if state.is_streaming:
        return jsonify({'success': False, 'error': 'Stream già attivo'})

    if not state.video_path:
        return jsonify({'success': False, 'error': 'Nessun video caricato'})

    data = request.json or {}
    state.loop = data.get('loop', True)

    # Ferma eventuali stream audio precedenti
    stop_audio_extraction()

    if not open_video(state.video_path):
        return jsonify({'success': False, 'error': state.error})

    state.is_streaming = True
    state.frame_count = 0  # Reset contatore frame
    state.status = "Streaming..."

    # Avvia thread streaming video
    thread = threading.Thread(target=stream_thread)
    thread.daemon = True
    thread.start()

    # Avvia audio dall'inizio (sincronizzato con video)
    start_audio_extraction(seek_time=0)

    return jsonify({'success': True})

@app.route('/api/stop', methods=['POST'])
def api_stop():
    """Ferma streaming video e audio"""
    state.is_streaming = False

    # Ferma video
    if state.video_capture:
        state.video_capture.release()
        state.video_capture = None

    # Ferma audio
    stop_audio_extraction()

    state.status = "Fermato"
    return jsonify({'success': True})

@app.route('/api/audio/mute', methods=['POST', 'GET'])
def api_audio_mute():
    """Controlla stato mute audio (chiamato da ESP32 quando cambia mode)"""
    if request.method == 'POST':
        data = request.get_json() or {}
        muted = data.get('muted', True)
        state.audio_muted = muted
        print(f"[AUDIO] Mute impostato a: {muted}")
        return jsonify({'success': True, 'muted': state.audio_muted})
    else:
        # GET: restituisce stato corrente
        return jsonify({'muted': state.audio_muted})

@app.route('/api/audio/output', methods=['POST', 'GET'])
def api_audio_output():
    """Seleziona output audio: 'browser' o 'esp32c3' (mutualmente esclusivi)"""
    if request.method == 'POST':
        data = request.get_json() or {}
        output = data.get('output', 'browser')
        if output in ['browser', 'esp32c3']:
            state.audio_output = output
            print(f"[AUDIO] Output selezionato: {output}")
            return jsonify({'success': True, 'output': state.audio_output})
        else:
            return jsonify({'success': False, 'error': 'Output non valido'})
    else:
        # GET: restituisce output corrente (usato da ESP32 per sapere se deve riprodurre)
        return jsonify({'output': state.audio_output})

@app.route('/api/video/looped', methods=['GET'])
def api_video_looped():
    """Controlla se il video e' appena ricominciato (loop).
    ESP32 lo usa per sapere quando riavviare l'audio sul C3.
    GET: restituisce looped=true se il video e' appena ricominciato, poi resetta il flag."""
    looped = state.video_looped
    if looped:
        state.video_looped = False  # Reset dopo lettura
    return jsonify({'looped': looped})

@app.route('/api/volume/c3', methods=['POST', 'GET'])
def api_volume_c3():
    """Controlla volume ESP32-C3.
    POST: imposta nuovo volume (0-100)
    GET: restituisce volume corrente e flag changed (per ESP32-S3)"""
    if request.method == 'POST':
        data = request.get_json() or {}
        volume = data.get('volume', 80)
        # Limita tra 0 e 95 (100 causa problemi audio su ESP32)
        volume = max(0, min(95, int(volume)))
        state.c3_volume = volume
        state.c3_volume_changed = True  # Segnala a ESP32-S3 di aggiornare
        print(f"[AUDIO] Volume C3 impostato a: {volume}%")
        return jsonify({'success': True, 'volume': state.c3_volume})
    else:
        # GET: restituisce volume e flag changed, poi resetta flag
        changed = state.c3_volume_changed
        if changed:
            state.c3_volume_changed = False
        return jsonify({'volume': state.c3_volume, 'changed': changed})

@app.route('/api/audio/delay', methods=['POST', 'GET'])
def api_audio_delay():
    """Controlla delay audio ESP32-C3 per sincronizzazione.
    POST: imposta nuovo delay (-5 a +5 secondi)
    GET: restituisce delay corrente"""
    if request.method == 'POST':
        data = request.get_json() or {}
        delay = data.get('delay', 0.0)
        # Limita tra -5 e +5 secondi
        delay = max(-5.0, min(5.0, float(delay)))
        state.c3_audio_delay = delay
        print(f"[AUDIO] Delay C3 impostato a: {delay:.1f}s")
        return jsonify({'success': True, 'delay': state.c3_audio_delay})
    else:
        return jsonify({'delay': state.c3_audio_delay})

@app.route('/api/clear_cache', methods=['POST'])
def api_clear_cache():
    """Elimina tutti i video dalla cache temp_videos"""
    try:
        # Prima ferma lo streaming se attivo
        if state.is_streaming:
            stop_streaming()

        # Conta e elimina i file
        files_deleted = 0
        total_size = 0

        if TEMP_DIR.exists():
            for file in TEMP_DIR.iterdir():
                if file.is_file():
                    try:
                        file_size = file.stat().st_size
                        file.unlink()
                        files_deleted += 1
                        total_size += file_size
                        print(f"[CACHE] Eliminato: {file.name}")
                    except Exception as e:
                        print(f"[CACHE] Errore eliminazione {file.name}: {e}")

        # Formatta dimensione
        if total_size > 1024 * 1024:
            size_str = f"{total_size / (1024 * 1024):.1f} MB"
        elif total_size > 1024:
            size_str = f"{total_size / 1024:.1f} KB"
        else:
            size_str = f"{total_size} bytes"

        message = f"Eliminati {files_deleted} file ({size_str})"
        print(f"[CACHE] {message}")
        return jsonify({'success': True, 'message': message, 'files': files_deleted, 'size': total_size})

    except Exception as e:
        print(f"[CACHE] Errore: {e}")
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/restart', methods=['POST'])
def api_restart():
    """Riavvia il server Python"""
    print("[SERVER] Riavvio richiesto via API...")

    def restart():
        time.sleep(0.5)  # Attendi che la risposta venga inviata
        print("[SERVER] Riavvio in corso...")
        # Su Windows usa subprocess per riavviare in un nuovo processo
        if sys.platform == 'win32':
            script_path = os.path.abspath(sys.argv[0])
            subprocess.Popen([sys.executable, script_path],
                           creationflags=subprocess.CREATE_NEW_CONSOLE)
            os._exit(0)  # Termina questo processo
        else:
            os.execv(sys.executable, [sys.executable] + sys.argv)

    # Avvia riavvio in thread separato per permettere risposta HTTP
    thread = threading.Thread(target=restart)
    thread.daemon = True
    thread.start()

    return jsonify({'success': True, 'message': 'Server in riavvio...'})

# ============================================================================
# MAIN
# ============================================================================

def kill_existing_server(port=5000):
    """Termina eventuali server già in esecuzione sulla porta specificata"""
    import socket

    # Verifica se la porta è già in uso
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    result = sock.connect_ex(('127.0.0.1', port))
    sock.close()

    if result == 0:
        print(f"[SERVER] Porta {port} già in uso, termino il processo esistente...")

        if sys.platform == 'win32':
            # Su Windows, trova e termina il processo che usa la porta
            try:
                # Trova il PID del processo
                result = subprocess.run(
                    f'netstat -ano | findstr ":{port}.*LISTENING"',
                    shell=True, capture_output=True, text=True
                )

                pids = set()
                for line in result.stdout.strip().split('\n'):
                    if line.strip():
                        parts = line.split()
                        if len(parts) >= 5:
                            pids.add(parts[-1])

                # Termina i processi trovati
                for pid in pids:
                    if pid.isdigit():
                        print(f"[SERVER] Terminazione processo PID {pid}...")
                        subprocess.run(f'taskkill /F /PID {pid}', shell=True,
                                      capture_output=True)

                # Attendi che la porta si liberi
                time.sleep(1)
                print("[SERVER] Processo precedente terminato")

            except Exception as e:
                print(f"[SERVER] Errore terminazione: {e}")
        else:
            # Su Linux/Mac usa fuser o lsof
            try:
                subprocess.run(f'fuser -k {port}/tcp', shell=True, capture_output=True)
                time.sleep(1)
            except:
                pass
    else:
        print(f"[SERVER] Porta {port} libera")

def main():
    print("=" * 60)
    print("  MJPEG STREAMING SERVER")
    print("  Per ESP32-4848S040")
    print("=" * 60)

    # Controlla e termina server esistenti
    kill_existing_server(SERVER_PORT)

    # Verifica dipendenze
    print("\n[CHECK] Verifica dipendenze...")

    if not check_ytdlp():
        print("[WARN] yt-dlp non trovato. Installa con: pip install yt-dlp")
        print("       Il download YouTube non funzionerà.")
    else:
        print("[OK] yt-dlp trovato")

    if not check_ffmpeg():
        print("[WARN] ffmpeg non trovato. Lo streaming audio non funzionerà.")
        print("       Installa ffmpeg e aggiungilo al PATH")
    else:
        print("[OK] ffmpeg trovato")

    print("[OK] OpenCV trovato")
    print("[OK] Flask trovato")

    # Ottieni IP locale
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
    except:
        local_ip = "127.0.0.1"

    print(f"\n[SERVER] Avvio su porta {SERVER_PORT}...")
    print(f"\n" + "=" * 60)
    print(f"  Interfaccia Web: http://localhost:{SERVER_PORT}")
    print(f"  URL Stream Video: http://{local_ip}:{SERVER_PORT}/stream")
    print(f"  URL Stream Audio: http://{local_ip}:{SERVER_PORT}/audio")
    print(f"=" * 60)
    print("\nUsa gli URL Stream nello sketch ESP32")
    print("Premi Ctrl+C per uscire\n")

    app.run(host='0.0.0.0', port=SERVER_PORT, threaded=True, debug=False)

if __name__ == '__main__':
    main()
