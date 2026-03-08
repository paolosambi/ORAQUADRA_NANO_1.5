#include "arcade_audio.h"

#ifdef ARCADE_DISABLE_AUDIO

// Stub implementations when audio is disabled
void ArcadeAudio::init() { }
void ArcadeAudio::cleanup() { }
void ArcadeAudio::start(machineBase *machine) { }
void ArcadeAudio::stop() { }
void ArcadeAudio::transmit() { }

#else

// ============================================================================
// Full audio implementation - Ported from Galag audio.cpp
// ESP32-S3 external I2S DAC on GPIO 1 (BCLK), 2 (LRC), 40 (DOUT)
// 3 synthesis engines: Namco WSG, AY-3-8910, DK PCM
//
// IMPORTANT: We do NOT install/uninstall the I2S driver. The Audio library
// owns it. We only change the sample rate and write stereo data directly.
// This avoids GDMA channel conflicts with the LCD_CAM RGB panel.
// ============================================================================

void ArcadeAudio::init() {
  if (active) return;

  // Audio library's I2S driver stays installed (stereo, 16-bit).
  // Configure sample rate to 24kHz for arcade synthesis.
  // Caller must have called audio.stopSong() + delay first.
  i2s_set_sample_rates(I2S_NUM_0, 24000);
  i2s_zero_dma_buffer(I2S_NUM_0);

  // STOP I2S DMA immediately after configuration.
  // DMA will only be started for games that actually produce audio.
  // Keeping I2S DMA running (even with silence) wastes GDMA bandwidth
  // which can conflict with LCD_CAM RGB panel DMA on ESP32-S3,
  // especially for ROT90 games (Bomb Jack) that are PSRAM-heavy.
  i2s_stop(I2S_NUM_0);

  active = true;
}

void ArcadeAudio::cleanup() {
  if (!active) return;

  active = false;
  currentMachine = nullptr;

  // Reset synthesis state
  memset(snd_buffer, 0, sizeof(snd_buffer));
  memset(ay_period, 0, sizeof(ay_period));
  memset(ay_volume, 0, sizeof(ay_volume));
  memset(ay_enable, 0, sizeof(ay_enable));
  memset(audio_cnt, 0, sizeof(audio_cnt));
  audio_toggle[0][0] = audio_toggle[0][1] = audio_toggle[0][2] = audio_toggle[0][3] = 1;
  audio_toggle[1][0] = audio_toggle[1][1] = audio_toggle[1][2] = audio_toggle[1][3] = 1;
  audio_toggle[2][0] = audio_toggle[2][1] = audio_toggle[2][2] = audio_toggle[2][3] = 1;
  ay_noise_rng[0] = ay_noise_rng[1] = ay_noise_rng[2] = 1;
  memset(ay_env_period, 0, sizeof(ay_env_period));
  memset(ay_env_shape, 0, sizeof(ay_env_shape));
  memset(ay_env_shape_latch, 0xFF, sizeof(ay_env_shape_latch));
  memset(ay_env_cnt, 0, sizeof(ay_env_cnt));
  memset(ay_env_step, 0, sizeof(ay_env_step));
  ay_env_up[0] = ay_env_up[1] = ay_env_up[2] = false;
  ay_env_holding[0] = ay_env_holding[1] = ay_env_holding[2] = false;
  ay_env_vol[0] = ay_env_vol[1] = ay_env_vol[2] = 0;
  snd_cnt[0] = snd_cnt[1] = snd_cnt[2] = 0;

  // Restart I2S so Audio library can use it when arcade mode exits
  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_start(I2S_NUM_0);
}

void ArcadeAudio::start(machineBase *machine) {
  currentMachine = machine;

  // Reset synthesis state for new game
  memset(ay_period, 0, sizeof(ay_period));
  memset(ay_volume, 0, sizeof(ay_volume));
  memset(ay_enable, 0, sizeof(ay_enable));
  memset(audio_cnt, 0, sizeof(audio_cnt));
  audio_toggle[0][0] = audio_toggle[0][1] = audio_toggle[0][2] = audio_toggle[0][3] = 1;
  audio_toggle[1][0] = audio_toggle[1][1] = audio_toggle[1][2] = audio_toggle[1][3] = 1;
  audio_toggle[2][0] = audio_toggle[2][1] = audio_toggle[2][2] = audio_toggle[2][3] = 1;
  ay_noise_rng[0] = ay_noise_rng[1] = ay_noise_rng[2] = 1;
  memset(ay_env_period, 0, sizeof(ay_env_period));
  memset(ay_env_shape, 0, sizeof(ay_env_shape));
  memset(ay_env_shape_latch, 0xFF, sizeof(ay_env_shape_latch));
  memset(ay_env_cnt, 0, sizeof(ay_env_cnt));
  memset(ay_env_step, 0, sizeof(ay_env_step));
  ay_env_up[0] = ay_env_up[1] = ay_env_up[2] = false;
  ay_env_holding[0] = ay_env_holding[1] = ay_env_holding[2] = false;
  ay_env_vol[0] = ay_env_vol[1] = ay_env_vol[2] = 0;
  snd_cnt[0] = snd_cnt[1] = snd_cnt[2] = 0;
  memset(snd_buffer, 0, sizeof(snd_buffer));

  // Only start I2S DMA for machines that produce audio.
  // Games without audio (Xevious) keep I2S stopped to save
  // GDMA bandwidth for the LCD_CAM RGB panel and PSRAM access.
  if (needsI2S()) {
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_start(I2S_NUM_0);
  }
}

void ArcadeAudio::stop() {
  // Stop I2S DMA FIRST — prevents any further DMA activity
  i2s_stop(I2S_NUM_0);

  // Clear machine pointer (signals transmit() to bail out)
  currentMachine = nullptr;

  // Zero audio buffer to prevent stale data leaking into next game
  memset(snd_buffer, 0, sizeof(snd_buffer));
}

void ArcadeAudio::transmit() {
  // Cache machine pointer locally: prevents race condition when stop()
  // runs on Core 1 while emulation task calls transmit() on Core 0.
  // Once cached, the pointer stays valid for this entire function call
  // because delete happens AFTER vTaskDelete + delay in arcadeStopGame().
  machineBase *m = currentMachine;
  if (!m || !active) return;

  // Skip entirely for machines without a supported audio engine.
  // I2S DMA is also stopped for these machines (see start()).
  signed char mt = m->machineType();
  bool isNamco = m->hasNamcoAudio();
  bool needsOutput = isNamco ||
    (mt == MCH_FROGGER || mt == MCH_1942 || mt == MCH_ANTEATER ||
     mt == MCH_LADYBUG || mt == MCH_DKONG || mt == MCH_BOMBJACK ||
     mt == MCH_GYRUSS);
  if (!needsOutput) return;

  // Try to transmit as much audio data as possible.
  // Safety limit prevents blocking on partial DMA writes.
  // At 60fps: 10 × 64 × 60 = 38400 samples/sec capacity (24kHz needed).
  size_t bytesOut = 0;
  int safety = 0;
  do {
    // Audio library uses stereo I2S (I2S_CHANNEL_FMT_RIGHT_LEFT).
    // Duplicate each mono sample to L+R channels.
    int16_t stereo_buf[128]; // 64 mono samples -> 128 stereo samples
    for (int i = 0; i < 64; i++) {
      stereo_buf[i * 2]     = snd_buffer[i];  // Left
      stereo_buf[i * 2 + 1] = snd_buffer[i];  // Right
    }

    i2s_write(I2S_NUM_0, stereo_buf, sizeof(stereo_buf), &bytesOut, 0);

    // Render next audio chunk if data was actually sent
    if (bytesOut) {
      if (!currentMachine) break;  // stop() was called — bail out
      if (isNamco) {
        namco_waveregs_parse();
        namco_render_buffer();
      } else {
        ay_render_buffer();
      }
    }
    if (++safety > 10) break;
  } while (bytesOut);
}

// ============================================================================
// AY-3-8910 + DK PCM renderer
// Handles: Frogger (1 AY), Anteater (1 AY), Bomb Jack (3 AY, sound CPU),
//          1942 (2 AY), Donkey Kong (PCM), Lady Bug (2 SN76489 mapped as 2 AY)
// ============================================================================
void ArcadeAudio::ay_render_buffer() {
  // Cache machine pointer — protects against race with stop() on other core
  machineBase *m = currentMachine;
  if (!m) return;

  signed char machineType = m->machineType();

  char AY, AY_INC, AY_VOL;
  if (machineType == MCH_BOMBJACK || machineType == MCH_GYRUSS) {
    AY = 3; AY_INC = 9; AY_VOL = 4;  // 3 chips: MAME route=0.13 each (vs 0.33 single-chip)
  } else if (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) {
    AY = 1; AY_INC = 9; AY_VOL = 11;
  } else if (machineType == MCH_LADYBUG) {
    AY = 2; AY_INC = 8; AY_VOL = 5;
  } else {
    AY = 2; AY_INC = 8; AY_VOL = 5;
  }

  if (machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER || machineType == MCH_LADYBUG || machineType == MCH_BOMBJACK || machineType == MCH_GYRUSS) {
    // Parse AY registers from cached machine pointer
    for (char ay = 0; ay < AY; ay++) {
      int ay_off = 16 * ay;

      // Three tone channels
      for (char c = 0; c < 3; c++) {
        ay_period[ay][c] = m->soundregs[ay_off + 2 * c] + 256 * (m->soundregs[ay_off + 2 * c + 1] & 15);
        ay_enable[ay][c] = (((m->soundregs[ay_off + 7] >> c) & 1) | ((m->soundregs[ay_off + 7] >> (c + 2)) & 2)) ^ 3;
        // Volume: bit 4 = envelope mode, bits 0-3 = fixed volume
        unsigned char volReg = m->soundregs[ay_off + 8 + c];
        ay_volume[ay][c] = (volReg & 0x10) ? ay_env_vol[ay] : (volReg & 0x0f);
      }
      // Noise channel
      ay_period[ay][3] = m->soundregs[ay_off + 6] & 0x1f;

      // Envelope: period from regs 11-12, shape from reg 13
      ay_env_period[ay] = m->soundregs[ay_off + 11] + 256 * m->soundregs[ay_off + 12];
      unsigned char newShape = m->soundregs[ay_off + 13] & 0x0F;
      if (newShape != ay_env_shape_latch[ay]) {
        // Register 13 was written: reset envelope generator
        ay_env_shape_latch[ay] = newShape;
        ay_env_shape[ay] = newShape;
        ay_env_step[ay] = 0;
        ay_env_cnt[ay] = 0;
        ay_env_holding[ay] = false;
        ay_env_up[ay] = (newShape & 0x04) != 0;  // ATT bit: 1=up, 0=down
      }
    }
  }

  // Render 64 samples
  for (int i = 0; i < 64; i++) {
    short value = 0;

    if (machineType == MCH_DKONG) {
      dkong *dkongMachine = static_cast<dkong*>(m);

      // Read from i8048 ring buffer if data available
      if (dkongMachine->dkong_audio_rptr != dkongMachine->dkong_audio_wptr) {
#ifdef ARCADE_WORKAROUND_I2S_APLL
        // APLL workaround: read at half rate (24kHz -> effective 12kHz)
        value = dkongMachine->dkong_audio_transfer_buffer[dkongMachine->dkong_audio_rptr]
                [(dkongMachine->dkong_obuf_toggle ? 32 : 0) + (i / 2)];
#else
        value = dkongMachine->dkong_audio_transfer_buffer[dkongMachine->dkong_audio_rptr][i];
#endif
      }

      // Include sample sounds (walk, jump, stomp)
      // walk = 6.25% volume, jump = 12.5%, stomp = 25%
      for (char j = 0; j < 3; j++) {
        if (dkongMachine->dkong_sample_cnt[j]) {
#ifdef ARCADE_WORKAROUND_I2S_APLL
          value += *dkongMachine->dkong_sample_ptr[j] >> (2 - j);
          if (i & 1) {
            dkongMachine->dkong_sample_ptr[j]++;
            dkongMachine->dkong_sample_cnt[j]--;
          }
#else
          value += *dkongMachine->dkong_sample_ptr[j]++ >> (2 - j);
          dkongMachine->dkong_sample_cnt[j]--;
#endif
        }
      }

#ifdef ARCADE_WORKAROUND_I2S_APLL
      if (i == 63) {
        // Advance ring buffer pointer
        if (dkongMachine->dkong_obuf_toggle)
          dkongMachine->dkong_audio_rptr = (dkongMachine->dkong_audio_rptr + 1) & DKONG_AUDIO_QUEUE_MASK;
        dkongMachine->dkong_obuf_toggle = !dkongMachine->dkong_obuf_toggle;
      }
#endif
    }
    else if (machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER || machineType == MCH_LADYBUG || machineType == MCH_BOMBJACK) {
      for (char ay = 0; ay < AY; ay++) {
        // Process noise generator
        if (ay_period[ay][3]) {
          audio_cnt[ay][3] += AY_INC;
          if (audio_cnt[ay][3] > ay_period[ay][3]) {
            audio_cnt[ay][3] -= ay_period[ay][3];
            ay_noise_rng[ay] ^= (((ay_noise_rng[ay] & 1) ^ ((ay_noise_rng[ay] >> 3) & 1)) << 17);
            ay_noise_rng[ay] >>= 1;
          }
        }

        // Advance envelope generator (16x slower than tone: period * 16)
        if (!ay_env_holding[ay]) {
          int envPer = ay_env_period[ay] ? ay_env_period[ay] * 16 : 16;
          ay_env_cnt[ay] += AY_INC;
          if (ay_env_cnt[ay] >= envPer) {
            ay_env_cnt[ay] -= envPer;
            ay_env_step[ay]++;
            if (ay_env_step[ay] >= 16) {
              // End of one cycle
              unsigned char sh = ay_env_shape[ay];
              if (!(sh & 0x08)) {
                // CONT=0: hold at 0
                ay_env_step[ay] = 0;
                ay_env_holding[ay] = true;
                ay_env_up[ay] = false;
              } else if (sh & 0x01) {
                // HOLD: stay at final value
                ay_env_step[ay] = 15;
                ay_env_holding[ay] = true;
                if (sh & 0x02) ay_env_up[ay] = !ay_env_up[ay]; // ALT
              } else {
                // Repeat
                ay_env_step[ay] = 0;
                if (sh & 0x02) ay_env_up[ay] = !ay_env_up[ay]; // ALT
              }
            }
          }
        }
        // Compute envelope volume (0-15)
        if (ay_env_holding[ay] && !(ay_env_shape[ay] & 0x08)) {
          ay_env_vol[ay] = 0;  // CONT=0: silent after decay
        } else {
          ay_env_vol[ay] = ay_env_up[ay] ? ay_env_step[ay] : (15 - ay_env_step[ay]);
        }

        // Re-read volume for envelope channels (envelope changes per sample)
        for (char c = 0; c < 3; c++) {
          int ay_off = 16 * ay;
          unsigned char volReg = m->soundregs[ay_off + 8 + c];
          if (volReg & 0x10) ay_volume[ay][c] = ay_env_vol[ay];
        }

        for (char c = 0; c < 3; c++) {
          if (ay_period[ay][c] && ay_volume[ay][c] && ay_enable[ay][c]) {
            short bit = 1;
            if (ay_enable[ay][c] & 1) bit &= (audio_toggle[ay][c] > 0) ? 1 : 0;  // tone
            if (ay_enable[ay][c] & 2) bit &= (ay_noise_rng[ay] & 1) ? 1 : 0;     // noise

            if (bit == 0) bit = -1;
            value += AY_VOL * bit * ay_volume[ay][c];

            audio_cnt[ay][c] += AY_INC;
            if (audio_cnt[ay][c] > ay_period[ay][c]) {
              audio_cnt[ay][c] -= ay_period[ay][c];
              audio_toggle[ay][c] = -audio_toggle[ay][c];
            }
          }
        }
      }
    }

    valueToBuffer(i, value);
  }
}

// ============================================================================
// Namco WSG (Waveform Sound Generator) - 3 channel wavetable synthesis
// Used by: Pac-Man, Galaga, Dig Dug, Eyes, Mr TNT, Liz Wiz, The Glob, Crush
// ============================================================================
void ArcadeAudio::namco_waveregs_parse() {
  machineBase *m = currentMachine;
  if (!m) return;
  for (char ch = 0; ch < 3; ch++) {
    snd_wave[ch] = m->waveRom(m->soundregs[ch * 5 + 0x05] & 0x07);
    snd_freq[ch] = (ch == 0) ? m->soundregs[0x10] : 0;
    snd_freq[ch] += m->soundregs[ch * 5 + 0x11] << 4;
    snd_freq[ch] += m->soundregs[ch * 5 + 0x12] << 8;
    snd_freq[ch] += m->soundregs[ch * 5 + 0x13] << 12;
    snd_freq[ch] += m->soundregs[ch * 5 + 0x14] << 16;
    snd_volume[ch] = m->soundregs[ch * 5 + 0x15];
  }
}

void ArcadeAudio::namco_render_buffer() {
  machineBase *m = currentMachine;
  if (!m) return;
  signed char machineType = m->machineType();

  for (int i = 0; i < 64; i++) {
    short value = 0;

    // Sum up to 3 wavetable channels
    if (snd_volume[0]) value += snd_volume[0] * snd_wave[0][(snd_cnt[0] >> 13) & 0x1f];
    if (snd_volume[1]) value += snd_volume[1] * snd_wave[1][(snd_cnt[1] >> 13) & 0x1f];
    if (snd_volume[2]) value += snd_volume[2] * snd_wave[2][(snd_cnt[2] >> 13) & 0x1f];

    snd_cnt[0] += snd_freq[0];
    snd_cnt[1] += snd_freq[1];
    snd_cnt[2] += snd_freq[2];

    // Galaga: overlay explosion sample
    if (machineType == MCH_GALAGA) {
      galaga *galagaMachine = static_cast<galaga*>(m);

      if (galagaMachine->snd_boom_cnt) {
        value += *galagaMachine->snd_boom_ptr * 3;

        if (galagaMachine->snd_boom_cnt & 1)
          galagaMachine->snd_boom_ptr++;

        galagaMachine->snd_boom_cnt--;
      }
    }

    valueToBuffer(i, value);
  }
}

// ============================================================================
// Convert synthesis value to I2S buffer sample
// External I2S DAC: signed 16-bit two's complement
// ============================================================================
void ArcadeAudio::valueToBuffer(int index, short value) {
  // Value is in range +/- 512, expand to +/- 15 bit
  value = value * 64;
  snd_buffer[index] = (int16_t)(value / volumeSetting);
}

#endif // ARCADE_DISABLE_AUDIO
