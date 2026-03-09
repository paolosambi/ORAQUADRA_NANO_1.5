// ============================================================================
// 53_BLE_GAMEPAD.ino - BLE Gamepad support for Arcade mode (NimBLE)
// Connects to BLE HID gamepads (8BitDo, Xbox BLE, generic) and maps to arcade
// Requires: NimBLE-Arduino library by h2zero (install from Library Manager)
// NOTA: WiFi viene spento in initArcade() prima di chiamare bleGamepadInit(),
//       quindi c'e' abbastanza heap per NimBLE + arcade emulator.
// ============================================================================

// Commenta la riga sotto per disabilitare il BLE Gamepad (se NimBLE non installato)
#define USE_NIMBLE

#ifdef EFFECT_ARCADE

#ifdef USE_NIMBLE
  #define BLE_GAMEPAD_AVAILABLE
  #include <NimBLEDevice.h>
  #pragma message "NimBLE found - BLE Gamepad ENABLED"
#else
  #pragma message "NimBLE NOT defined - BLE Gamepad DISABLED"
#endif

#ifdef BLE_GAMEPAD_AVAILABLE

extern volatile bool arcadeBubbleActive;
extern volatile uint8_t arcadeButtonState;

// ====== Config ======
#define BLE_GP_SCAN_TIME      0       // 0 = scan indefinitely
#define BLE_GP_STICK_DEADZONE 64      // Threshold stick axis (0-255, center=128)
#define BLE_GP_HID_SVC        "1812"  // HID Service UUID
#define BLE_GP_REPORT_CHR     "2a4d"  // HID Report characteristic UUID

// ====== State ======
static volatile uint8_t bleGpButtons = 0;
static bool bleGpConnected = false;
static volatile bool bleGpInitialized = false;
static volatile bool bleGpInitRequested = false;  // Background init task launched
static NimBLEClient* bleGpClient = nullptr;
static NimBLEAddress* bleGpTargetAddr = nullptr;
static bool bleGpConnecting = false;

// ====== Format Detection ======
enum BleGpFormat { FMT_UNKNOWN, FMT_STANDARD_8, FMT_XBOX_BLE, FMT_LONG_GENERIC };
static BleGpFormat bleGpFormat = FMT_UNKNOWN;

// ====== Hat Switch Parser ======
// Standard hat: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW, 8/0x0F/0xFF=center
static uint8_t bleGpParseHat(uint8_t hat) {
  uint8_t b = 0;
  if (hat == 0 || hat == 1 || hat == 7) b |= BUTTON_UP;
  if (hat == 2 || hat == 3 || hat == 1) b |= BUTTON_RIGHT;
  if (hat == 4 || hat == 5 || hat == 3) b |= BUTTON_DOWN;
  if (hat == 6 || hat == 7 || hat == 5) b |= BUTTON_LEFT;
  return b;
}

// ====== Stick Axis to Buttons ======
static uint8_t bleGpParseStick8(uint8_t x, uint8_t y) {
  uint8_t b = 0;
  if (x < (128 - BLE_GP_STICK_DEADZONE)) b |= BUTTON_LEFT;
  if (x > (128 + BLE_GP_STICK_DEADZONE)) b |= BUTTON_RIGHT;
  if (y < (128 - BLE_GP_STICK_DEADZONE)) b |= BUTTON_UP;
  if (y > (128 + BLE_GP_STICK_DEADZONE)) b |= BUTTON_DOWN;
  return b;
}

static uint8_t bleGpParseStick16(int16_t x, int16_t y) {
  uint8_t b = 0;
  // 16-bit axes: center ~0 or ~32768, range varies
  // Normalize to 0-255 range then apply deadzone
  uint8_t nx = (uint8_t)((x >> 8) + 128);
  uint8_t ny = (uint8_t)((y >> 8) + 128);
  return bleGpParseStick8(nx, ny);
}

// ====== HID Report Parser (auto-detect format) ======
static uint8_t bleGpParseReport(const uint8_t* data, size_t len) {
  if (len < 2) return 0;

  uint8_t buttons = 0;

  // Auto-detect format on first valid report
  if (bleGpFormat == FMT_UNKNOWN) {
    if (len >= 14 && data[0] == 0x01) {
      bleGpFormat = FMT_XBOX_BLE;
      Serial.printf("[BLE-GP] Formato rilevato: Xbox BLE (%d bytes)\n", len);
    } else if (len == 8) {
      bleGpFormat = FMT_STANDARD_8;
      Serial.printf("[BLE-GP] Formato rilevato: Standard 8-byte\n");
    } else if (len >= 10) {
      bleGpFormat = FMT_LONG_GENERIC;
      Serial.printf("[BLE-GP] Formato rilevato: Generic long (%d bytes)\n", len);
    } else {
      // Try generic parsing for short reports
      bleGpFormat = FMT_STANDARD_8;
      Serial.printf("[BLE-GP] Formato fallback: Standard (%d bytes)\n", len);
    }
  }

  switch (bleGpFormat) {

    case FMT_STANDARD_8: {
      // [LX][LY][RX][RY][hat+btns][btns][L2][R2]
      // 8BitDo D-input, many generic HID gamepads
      if (len >= 5) {
        // Left stick
        buttons |= bleGpParseStick8(data[0], data[1]);
        // Hat switch (lower nibble of byte 4) + button bits
        uint8_t hatBtns = data[4];
        buttons |= bleGpParseHat(hatBtns & 0x0F);
        // Action buttons (upper nibble byte 4 + byte 5)
        // Bit layout varies, common: b4.4=A, b4.5=B, b4.6=X, b4.7=Y
        if (hatBtns & 0x10) buttons |= BUTTON_FIRE;   // A / Cross
        if (hatBtns & 0x20) buttons |= BUTTON_EXTRA;  // B / Circle
        if (len >= 6) {
          uint8_t btns2 = data[5];
          // b5.4=Select, b5.5=Start (common layout)
          if (btns2 & 0x10) buttons |= BUTTON_COIN;
          if (btns2 & 0x20) buttons |= BUTTON_START;
          // Also check lower bits for some controllers
          if (btns2 & 0x04) buttons |= BUTTON_COIN;   // Select alt
          if (btns2 & 0x08) buttons |= BUTTON_START;  // Start alt
        }
      }
      break;
    }

    case FMT_XBOX_BLE: {
      // [0x01][btns_lo][btns_hi][hat][LX_lo][LX_hi][LY_lo][LY_hi]...
      // Xbox Wireless Controller in BLE mode
      if (len >= 14) {
        uint8_t btnsLo = data[1];
        uint8_t btnsHi = data[2];
        uint8_t hat = data[3];

        // Hat switch
        buttons |= bleGpParseHat(hat);

        // Left stick (16-bit signed, bytes 4-7)
        int16_t lx = (int16_t)(data[4] | (data[5] << 8));
        int16_t ly = (int16_t)(data[6] | (data[7] << 8));
        buttons |= bleGpParseStick16(lx, ly);

        // Xbox button mapping:
        // btnsLo: 0=A, 1=B, 2=X, 3=Y, 4=LB, 5=RB, 6=Back/View, 7=Start/Menu
        if (btnsLo & 0x01) buttons |= BUTTON_FIRE;   // A
        if (btnsLo & 0x02) buttons |= BUTTON_EXTRA;  // B
        if (btnsLo & 0x04) buttons |= BUTTON_FIRE;   // X (also fire)
        if (btnsLo & 0x40) buttons |= BUTTON_COIN;   // Back/View = Coin
        if (btnsLo & 0x80) buttons |= BUTTON_START;  // Start/Menu
      }
      break;
    }

    case FMT_LONG_GENERIC: {
      // [LX][LY][RX][RY][btns1][btns2][hat][...]
      // Generic long-format HID gamepads
      if (len >= 7) {
        // Left stick
        buttons |= bleGpParseStick8(data[0], data[1]);
        // Buttons
        uint8_t btns1 = data[4];
        uint8_t btns2 = data[5];
        uint8_t hat = data[6];

        buttons |= bleGpParseHat(hat);

        if (btns1 & 0x01) buttons |= BUTTON_FIRE;   // A
        if (btns1 & 0x02) buttons |= BUTTON_EXTRA;  // B
        if (btns2 & 0x10) buttons |= BUTTON_COIN;   // Select
        if (btns2 & 0x20) buttons |= BUTTON_START;  // Start
      }
      break;
    }

    default:
      break;
  }

  return buttons;
}

// ====== Notification Callback (called from BLE task) ======
static void bleGpNotifyCallback(NimBLERemoteCharacteristic* chr,
                                 uint8_t* data, size_t len, bool isNotify) {
  bleGpButtons = bleGpParseReport(data, len);
}

// ====== Scan Callbacks - Find HID devices ======
class BleGpScanCB : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    if (dev->isAdvertisingService(NimBLEUUID(BLE_GP_HID_SVC))) {
      Serial.printf("[BLE-GP] Trovato: %s (%s)\n",
                    dev->getName().c_str(), dev->getAddress().toString().c_str());
      bleGpTargetAddr = new NimBLEAddress(dev->getAddress());
      NimBLEDevice::getScan()->stop();
    }
  }
};

// ====== Client Callbacks - Connection management ======
class BleGpClientCB : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* client) override {
    bleGpConnected = true;
    Serial.println("[BLE-GP] Connesso!");
  }
  void onDisconnect(NimBLEClient* client, int reason) override {
    bleGpConnected = false;
    bleGpButtons = 0;
    bleGpFormat = FMT_UNKNOWN;
    Serial.printf("[BLE-GP] Disconnesso (reason=%d)\n", reason);
    // Auto-reconnect: restart scan if still in arcade
    if (arcadeBubbleActive && bleGpInitialized) {
      NimBLEDevice::getScan()->start(BLE_GP_SCAN_TIME, false);
    }
  }
};

// ====== Connect to found gamepad ======
static void bleGpTryConnect() {
  if (!bleGpTargetAddr) return;

  Serial.printf("[BLE-GP] Connessione a %s...\n", bleGpTargetAddr->toString().c_str());

  bleGpClient = NimBLEDevice::createClient();
  bleGpClient->setClientCallbacks(new BleGpClientCB(), false);
  bleGpClient->setConnectionParams(6, 12, 0, 200); // Fast connection interval

  if (!bleGpClient->connect(*bleGpTargetAddr)) {
    Serial.println("[BLE-GP] Connessione fallita");
    NimBLEDevice::deleteClient(bleGpClient);
    bleGpClient = nullptr;
    delete bleGpTargetAddr;
    bleGpTargetAddr = nullptr;
    // Restart scan
    if (bleGpInitialized) {
      NimBLEDevice::getScan()->start(BLE_GP_SCAN_TIME, false);
    }
    return;
  }

  // Discover HID Service
  NimBLERemoteService* svc = bleGpClient->getService(NimBLEUUID(BLE_GP_HID_SVC));
  if (!svc) {
    Serial.println("[BLE-GP] HID Service non trovato");
    bleGpClient->disconnect();
    return;
  }

  // Subscribe to all Report characteristics with notify
  // NimBLE v2.x: getCharacteristics() returns const vector& (not pointer)
  int subscribed = 0;
  const auto& chars = svc->getCharacteristics(true);
  for (auto& chr : chars) {
    if (chr->getUUID() == NimBLEUUID(BLE_GP_REPORT_CHR)) {
      if (chr->canNotify()) {
        if (chr->subscribe(true, bleGpNotifyCallback)) {
          subscribed++;
          Serial.printf("[BLE-GP] Subscribed report char handle=%d\n", chr->getHandle());
        }
      }
    }
  }

  if (subscribed == 0) {
    Serial.println("[BLE-GP] Nessun report characteristic trovato!");
    bleGpClient->disconnect();
    return;
  }

  Serial.printf("[BLE-GP] Pronto! %d report(s) sottoscritti\n", subscribed);
}

// ====== Background connection task (FreeRTOS, Core 0) ======
static void bleGpConnectTask(void* param) {
  bleGpTryConnect();
  bleGpConnecting = false;
  vTaskDelete(NULL);
}

// ====== Background BLE init task (avoids blocking render) ======
static void bleGpInitTask(void* param) {
  Serial.printf("[BLE-GP] Init task, heap: %d\n", ESP.getFreeHeap());
  NimBLEDevice::init("OraQuadra");  // Can take 200-500ms (BLE stack init)
  Serial.printf("[BLE-GP] NimBLE init OK, heap: %d\n", ESP.getFreeHeap());
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Max range

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new BleGpScanCB(), false);
  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(15);
  scan->start(BLE_GP_SCAN_TIME, false);  // Non-blocking async scan

  bleGpInitialized = true;
  Serial.println("[BLE-GP] Scan avviato - cercando gamepad BLE...");
  vTaskDelete(NULL);  // One-shot task, self-delete
}

// ====== Public API ======

void bleGamepadInit() {
  if (bleGpInitialized || bleGpInitRequested) return;
  bleGpInitRequested = true;
  Serial.printf("[BLE-GP] Init BLE, heap: %d bytes\n", ESP.getFreeHeap());
  // Task con stack da heap interno (8KB) - NimBLEDevice::init() ha bisogno di stack
  xTaskCreatePinnedToCore(bleGpInitTask, "bleGpInit", 8192, NULL, 1, NULL, 0);
}

void bleGamepadUpdate() {
  if (!bleGpInitialized) return;
  if (bleGpTargetAddr && !bleGpConnected && !bleGpConnecting) {
    bleGpConnecting = true;
    xTaskCreatePinnedToCore(bleGpConnectTask, "bleGpConn", 4096, NULL, 1, NULL, 0);
  }
}

void bleGamepadCleanup() {
  // If background init task is still running, wait for it (max 3s)
  if (bleGpInitRequested && !bleGpInitialized) {
    Serial.println("[BLE-GP] Waiting for init task to finish...");
    for (int i = 0; i < 300 && !bleGpInitialized; i++) {
      delay(10);
    }
  }
  bleGpInitRequested = false;

  if (!bleGpInitialized) return;

  // Ferma lo scan PRIMA di disconnettere (evita callback durante cleanup)
  NimBLEDevice::getScan()->stop();
  delay(50);  // Attendi che lo scan si fermi completamente

  // Disconnetti client se connesso
  if (bleGpClient && bleGpClient->isConnected()) {
    bleGpClient->disconnect();
    delay(100);  // Attendi disconnessione completa
  }

  // Attendi che eventuali task di connessione in background terminino
  if (bleGpConnecting) {
    Serial.println("[BLE-GP] Waiting for connect task...");
    for (int i = 0; i < 200 && bleGpConnecting; i++) {
      delay(10);
    }
  }

  // Reset state PRIMA di deinit (le callback non accederanno piu' a NimBLE)
  bleGpConnected = false;
  bleGpButtons = 0;
  bleGpInitialized = false;  // Impedisce nuovi scan/connessioni dalle callback
  bleGpFormat = FMT_UNKNOWN;
  bleGpConnecting = false;
  bleGpClient = nullptr;
  if (bleGpTargetAddr) { delete bleGpTargetAddr; bleGpTargetAddr = nullptr; }

  // Deinit BLE stack - rilascia tutta la memoria
  NimBLEDevice::deinit(true);
  delay(50);  // Attendi rilascio risorse

  Serial.printf("[BLE-GP] Cleanup completato, heap: %d bytes\n", ESP.getFreeHeap());
}

uint8_t bleGamepadGetButtons() { return bleGpButtons; }
bool bleGamepadIsConnected() { return bleGpConnected; }

#else // !BLE_GAMEPAD_AVAILABLE

// Stubs when NimBLE is not installed
void bleGamepadInit() {}
void bleGamepadCleanup() {}
void bleGamepadUpdate() {}
uint8_t bleGamepadGetButtons() { return 0; }
bool bleGamepadIsConnected() { return false; }

#endif // BLE_GAMEPAD_AVAILABLE

#endif // EFFECT_ARCADE
