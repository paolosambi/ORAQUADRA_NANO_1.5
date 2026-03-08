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

  // Check if current machine needs I2S audio output
  bool needsI2S() {
    if (!currentMachine) return false;
    if (currentMachine->hasNamcoAudio()) return true;
    signed char mt = currentMachine->machineType();
    return (mt == MCH_FROGGER || mt == MCH_1942 || mt == MCH_ANTEATER ||
            mt == MCH_LADYBUG || mt == MCH_DKONG || mt == MCH_BOMBJACK ||
            mt == MCH_GYRUSS);
  }

  machineBase *currentMachine = nullptr;
  bool active = false;

  // Mono synthesis buffer (duplicated to stereo before i2s_write)
  int16_t snd_buffer[64];

  // AY-3-8910 state (up to 3 chips: Bomb Jack uses 3)
  int ay_period[3][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
  int ay_volume[3][3] = {{0,0,0}, {0,0,0}, {0,0,0}};
  int ay_enable[3][3] = {{0,0,0}, {0,0,0}, {0,0,0}};
  int audio_cnt[3][4];
  int audio_toggle[3][4] = {{1,1,1,1}, {1,1,1,1}, {1,1,1,1}};
  unsigned long ay_noise_rng[3] = {1, 1, 1};

  // AY-3-8910 envelope generator (one per chip)
  int ay_env_period[3] = {0, 0, 0};       // Period from regs 11-12
  unsigned char ay_env_shape[3] = {0, 0, 0}; // Shape from reg 13
  unsigned char ay_env_shape_latch[3] = {0xFF, 0xFF, 0xFF}; // Detect reg 13 writes
  int ay_env_cnt[3] = {0, 0, 0};          // Tick counter
  int ay_env_step[3] = {0, 0, 0};         // Position 0-15
  bool ay_env_up[3] = {false, false, false};     // Direction
  bool ay_env_holding[3] = {false, false, false}; // Holding at end
  unsigned char ay_env_vol[3] = {0, 0, 0};       // Current envelope volume 0-15

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
