#ifndef ARCADE_AUDIO_H
#define ARCADE_AUDIO_H

#include "arcade_config.h"
#include "machines/machineBase.h"

#ifndef ARCADE_DISABLE_AUDIO

#include <driver/i2s.h>

// ESP-IDF >= 4.4.4: APLL is broken, use workaround (run DK at 24kHz, half-rate buffer)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 4)
#define ARCADE_WORKAROUND_I2S_APLL
#endif

// Forward declarations for static_cast in render
#include "machines/dkong/dkong.h"
#include "machines/galaga/galaga.h"

#endif // !ARCADE_DISABLE_AUDIO

class ArcadeAudio {
public:
  void init();
  void cleanup();
  void start(machineBase *machine);
  void stop();
  void transmit();

#ifndef ARCADE_DISABLE_AUDIO
private:
  void namco_waveregs_parse();
  void namco_render_buffer();
  void ay_render_buffer();
  void valueToBuffer(int index, short value);

  machineBase *currentMachine = nullptr;
  bool active = false;

  // Mono synthesis buffer (duplicated to stereo before i2s_write)
  int16_t snd_buffer[64];

  // AY-3-8910 state (up to 2 chips)
  int ay_period[2][4] = {{0,0,0,0}, {0,0,0,0}};
  int ay_volume[2][3] = {{0,0,0}, {0,0,0}};
  int ay_enable[2][3] = {{0,0,0}, {0,0,0}};
  int audio_cnt[2][4];
  int audio_toggle[2][4] = {{1,1,1,1}, {1,1,1,1}};
  unsigned long ay_noise_rng[2] = {1, 1};

  // Namco WSG state (3 channels)
  unsigned long snd_cnt[3] = {0, 0, 0};
  unsigned long snd_freq[3];
  const signed char *snd_wave[3];
  unsigned char snd_volume[3];

  // Volume divider (lower = louder, 1-30)
  short volumeSetting = 3;
#endif // !ARCADE_DISABLE_AUDIO
};

#endif // ARCADE_AUDIO_H
