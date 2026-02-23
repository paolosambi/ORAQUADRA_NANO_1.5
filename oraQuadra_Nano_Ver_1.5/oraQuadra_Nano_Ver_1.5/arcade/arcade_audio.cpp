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
  // Just change sample rate to 24kHz for arcade synthesis.
  // Caller must have called audio.stopSong() + delay first.
  i2s_set_sample_rates(I2S_NUM_0, 24000);
  i2s_zero_dma_buffer(I2S_NUM_0);
  active = true;
  Serial.println("[ARCADE_AUDIO] Active (24kHz stereo)");
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
  ay_noise_rng[0] = ay_noise_rng[1] = 1;
  snd_cnt[0] = snd_cnt[1] = snd_cnt[2] = 0;

  // Silence DMA buffer, don't touch I2S driver
  // Audio library will reconfigure sample rate on next playback
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("[ARCADE_AUDIO] Cleaned up");
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
  ay_noise_rng[0] = ay_noise_rng[1] = 1;
  snd_cnt[0] = snd_cnt[1] = snd_cnt[2] = 0;
  memset(snd_buffer, 0, sizeof(snd_buffer));

  Serial.printf("[ARCADE_AUDIO] Started for machine type %d\n", machine->machineType());
}

void ArcadeAudio::stop() {
  currentMachine = nullptr;
}

void ArcadeAudio::transmit() {
  if (!currentMachine || !active) return;

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
      if (currentMachine->hasNamcoAudio()) {
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
// Handles: Frogger (1 AY), Anteater (1 AY), 1942 (2 AY), Donkey Kong (PCM)
// ============================================================================
void ArcadeAudio::ay_render_buffer() {
  signed char machineType = currentMachine->machineType();

  char AY = (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) ? 1 : 2;
  char AY_INC = (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) ? 9 : 8;
  char AY_VOL = (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) ? 11 : 5;

  if (machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER) {
    // Parse AY registers
    for (char ay = 0; ay < AY; ay++) {
      int ay_off = 16 * ay;

      // Three tone channels
      for (char c = 0; c < 3; c++) {
        ay_period[ay][c] = currentMachine->soundregs[ay_off + 2 * c] + 256 * (currentMachine->soundregs[ay_off + 2 * c + 1] & 15);
        ay_enable[ay][c] = (((currentMachine->soundregs[ay_off + 7] >> c) & 1) | ((currentMachine->soundregs[ay_off + 7] >> (c + 2)) & 2)) ^ 3;
        ay_volume[ay][c] = currentMachine->soundregs[ay_off + 8 + c] & 0x0f;
      }
      // Noise channel
      ay_period[ay][3] = currentMachine->soundregs[ay_off + 6] & 0x1f;
    }
  }

  // Render 64 samples
  for (int i = 0; i < 64; i++) {
    short value = 0;

    if (machineType == MCH_DKONG) {
      dkong *dkongMachine = static_cast<dkong*>(currentMachine);

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
    else if (machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER) {
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
  for (char ch = 0; ch < 3; ch++) {
    snd_wave[ch] = currentMachine->waveRom(currentMachine->soundregs[ch * 5 + 0x05] & 0x07);
    snd_freq[ch] = (ch == 0) ? currentMachine->soundregs[0x10] : 0;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x11] << 4;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x12] << 8;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x13] << 12;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x14] << 16;
    snd_volume[ch] = currentMachine->soundregs[ch * 5 + 0x15];
  }
}

void ArcadeAudio::namco_render_buffer() {
  signed char machineType = currentMachine->machineType();

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
      galaga *galagaMachine = static_cast<galaga*>(currentMachine);

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
