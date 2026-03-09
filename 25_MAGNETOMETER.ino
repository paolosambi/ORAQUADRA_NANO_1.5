// ================== MAGNETOMETRO (QMC5883P) ==================
/*
 * Driver per magnetometro QMC5883P usando libreria Adafruit_QMC5883P
 * Con calibrazione salvata in Preferences (NVS)
 *
 * Connessioni I2C:
 * - SDA: Pin 19
 * - SCL: Pin 45
 * - VCC: 3.3V
 * - GND: GND
 *
 * Indirizzo: 0x0C (il tuo chip) o 0x2C (default)
 */

#include <Adafruit_QMC5883P.h>
#include <Preferences.h>

// ================== VARIABILI GLOBALI ==================
bool magnetometerConnected = false;
float magnetometerHeading = 0.0;
int16_t magX = 0, magY = 0, magZ = 0;
float magneticDeclination = 3.0;  // Declinazione Italia ~3°

// Oggetto magnetometro
Adafruit_QMC5883P qmc;

// Preferences per salvare calibrazione
Preferences magPrefs;

// Indirizzo trovato
uint8_t magAddress = 0;

// ================== CALIBRAZIONE ==================
float magOffsetX = 0;
float magOffsetY = 0;
float magOffsetZ = 0;
float magScaleX = 1.0;
float magScaleY = 1.0;
float magScaleZ = 1.0;
bool magCalibrated = false;

// Min/Max per calibrazione
int16_t magMinX = 32767, magMaxX = -32768;
int16_t magMinY = 32767, magMaxY = -32768;
int16_t magMinZ = 32767, magMaxZ = -32768;

// ================== SCAN I2C BUS ==================
void scanI2CBus() {
  Serial.println("[MAG] Scansione bus I2C...");
  int deviceCount = 0;

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  [I2C] 0x%02X", address);
      if (address == 0x5D || address == 0x14) Serial.print(" (GT911 Touch)");
      else if (address == 0x0C || address == 0x2C) Serial.print(" (QMC5883P Magnetometer)");
      else if (address == 0x0D) Serial.print(" (QMC5883L)");
      else if (address == 0x1E) Serial.print(" (HMC5883L)");
      else if (address == 0x76 || address == 0x77) Serial.print(" (BME280)");
      else if (address == 0x68) Serial.print(" (MPU6050/DS3231)");
      Serial.println();
      deviceCount++;
    }
  }
  Serial.printf("  [I2C] Totale: %d dispositivi\n", deviceCount);
}

// ================== SALVA CALIBRAZIONE ==================
void saveCalibration() {
  magPrefs.begin("magnetometer", false);
  magPrefs.putFloat("offsetX", magOffsetX);
  magPrefs.putFloat("offsetY", magOffsetY);
  magPrefs.putFloat("offsetZ", magOffsetZ);
  magPrefs.putFloat("scaleX", magScaleX);
  magPrefs.putFloat("scaleY", magScaleY);
  magPrefs.putFloat("scaleZ", magScaleZ);
  magPrefs.putBool("calibrated", true);
  magPrefs.end();
  Serial.println("[MAG] Calibrazione salvata");
}

// ================== CARICA CALIBRAZIONE ==================
bool loadCalibration() {
  magPrefs.begin("magnetometer", true);
  bool hasCalibration = magPrefs.getBool("calibrated", false);

  if (hasCalibration) {
    magOffsetX = magPrefs.getFloat("offsetX", 0);
    magOffsetY = magPrefs.getFloat("offsetY", 0);
    magOffsetZ = magPrefs.getFloat("offsetZ", 0);
    magScaleX = magPrefs.getFloat("scaleX", 1.0);
    magScaleY = magPrefs.getFloat("scaleY", 1.0);
    magScaleZ = magPrefs.getFloat("scaleZ", 1.0);
    magCalibrated = true;

    Serial.println("[MAG] Calibrazione caricata:");
    Serial.printf("  Offset: X=%.1f Y=%.1f Z=%.1f\n", magOffsetX, magOffsetY, magOffsetZ);
    Serial.printf("  Scale:  X=%.2f Y=%.2f Z=%.2f\n", magScaleX, magScaleY, magScaleZ);
  }

  magPrefs.end();
  return hasCalibration;
}

// ================== DIREZIONE CARDINALE ==================
const char* getCardinalDirection(float heading) {
  if (heading >= 337.5 || heading < 22.5) return "N";
  if (heading >= 22.5 && heading < 67.5) return "NE";
  if (heading >= 67.5 && heading < 112.5) return "E";
  if (heading >= 112.5 && heading < 157.5) return "SE";
  if (heading >= 157.5 && heading < 202.5) return "S";
  if (heading >= 202.5 && heading < 247.5) return "SW";
  if (heading >= 247.5 && heading < 292.5) return "W";
  if (heading >= 292.5 && heading < 337.5) return "NW";
  return "?";
}

// ================== CALIBRAZIONE GUIDATA ==================
void calibrateMagnetometerGuided() {
  Serial.println("\n[MAG] === CALIBRAZIONE MAGNETOMETRO ===");

  gfx->fillScreen(0x0000);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(0x07FF);
  gfx->setCursor(80, 50);
  gfx->print("CALIBRAZIONE");
  gfx->setCursor(100, 80);
  gfx->print("BUSSOLA");

  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(40, 130);
  gfx->print("Ruota LENTAMENTE il dispositivo");
  gfx->setCursor(60, 155);
  gfx->print("in TUTTE le direzioni");

  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(0xFFE0);
  gfx->setCursor(100, 200);
  gfx->print("Fai un 8 nell'aria!");

  gfx->drawCircle(200, 280, 30, 0xF800);
  gfx->drawCircle(280, 280, 30, 0xF800);

  // Reset min/max
  magMinX = 32767; magMaxX = -32768;
  magMinY = 32767; magMaxY = -32768;
  magMinZ = 32767; magMaxZ = -32768;

  unsigned long startTime = millis();
  unsigned long lastUpdate = 0;
  int totalTime = 15;

  while (millis() - startTime < totalTime * 1000) {
    int16_t x, y, z;
    if (qmc.getRawMagnetic(&x, &y, &z)) {
      if (x < magMinX) magMinX = x;
      if (x > magMaxX) magMaxX = x;
      if (y < magMinY) magMinY = y;
      if (y > magMaxY) magMaxY = y;
      if (z < magMinZ) magMinZ = z;
      if (z > magMaxZ) magMaxZ = z;
    }

    if (millis() - lastUpdate >= 500) {
      lastUpdate = millis();
      int remaining = totalTime - (millis() - startTime) / 1000;

      int progress = ((millis() - startTime) * 400) / (totalTime * 1000);
      gfx->fillRect(40, 350, 400, 30, 0x2104);
      gfx->fillRect(40, 350, progress, 30, 0x07E0);

      gfx->fillRect(180, 380, 120, 40, 0x0000);
      gfx->setFont(u8g2_font_helvB24_tn);
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(220, 415);
      gfx->printf("%d", remaining);

      gfx->fillRect(40, 240, 400, 20, 0x0000);
      gfx->setFont(u8g2_font_helvR10_tr);
      gfx->setTextColor(0x8410);
      gfx->setCursor(40, 255);
      gfx->printf("X:[%d,%d] Y:[%d,%d]", magMinX, magMaxX, magMinY, magMaxY);

      Serial.printf("[MAG] %ds X:[%d,%d] Y:[%d,%d] Z:[%d,%d]\n",
                    remaining, magMinX, magMaxX, magMinY, magMaxY, magMinZ, magMaxZ);
    }

    delay(20);
  }

  // Calcola offset (centro)
  magOffsetX = (magMaxX + magMinX) / 2.0;
  magOffsetY = (magMaxY + magMinY) / 2.0;
  magOffsetZ = (magMaxZ + magMinZ) / 2.0;

  // Calcola scale
  float avgDeltaX = (magMaxX - magMinX) / 2.0;
  float avgDeltaY = (magMaxY - magMinY) / 2.0;
  float avgDeltaZ = (magMaxZ - magMinZ) / 2.0;
  float avgDelta = (avgDeltaX + avgDeltaY + avgDeltaZ) / 3.0;

  if (avgDeltaX > 0) magScaleX = avgDelta / avgDeltaX;
  if (avgDeltaY > 0) magScaleY = avgDelta / avgDeltaY;
  if (avgDeltaZ > 0) magScaleZ = avgDelta / avgDeltaZ;

  magCalibrated = true;
  saveCalibration();

  gfx->fillScreen(0x0000);
  gfx->setFont(u8g2_font_helvB24_tr);
  gfx->setTextColor(0x07E0);
  gfx->setCursor(80, 180);
  gfx->print("CALIBRAZIONE");
  gfx->setCursor(100, 220);
  gfx->print("COMPLETATA!");

  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(60, 280);
  gfx->printf("Offset: X=%.0f Y=%.0f Z=%.0f", magOffsetX, magOffsetY, magOffsetZ);

  Serial.println("\n[MAG] === CALIBRAZIONE COMPLETATA ===");
  Serial.printf("[MAG] Offset: X=%.1f Y=%.1f Z=%.1f\n", magOffsetX, magOffsetY, magOffsetZ);
  Serial.printf("[MAG] Scale:  X=%.2f Y=%.2f Z=%.2f\n", magScaleX, magScaleY, magScaleZ);

  delay(3000);
}

// ================== SETUP MAGNETOMETRO ==================
void setup_magnetometer() {
  Serial.println("\n========================================");
  Serial.println("INIZIALIZZAZIONE MAGNETOMETRO QMC5883P");
  Serial.println("========================================");

  scanI2CBus();

  // Prova indirizzi: 0x0C (il tuo) e 0x2C (default)
  uint8_t addresses[] = {0x0C, 0x2C};
  bool found = false;

  for (int i = 0; i < 2; i++) {
    Serial.printf("\n[MAG] Provo indirizzo 0x%02X...\n", addresses[i]);
    if (qmc.begin(addresses[i], &Wire)) {
      magAddress = addresses[i];
      found = true;
      Serial.printf("[MAG] QMC5883P trovato a 0x%02X!\n", magAddress);
      break;
    }
  }

  if (!found) {
    magnetometerConnected = false;
    Serial.println("\n[MAG] ERRORE: QMC5883P NON trovato!");
    Serial.println("========================================\n");
    return;
  }

  // Configura il sensore
  qmc.setMode(QMC5883P_MODE_CONTINUOUS);
  qmc.setODR(QMC5883P_ODR_100HZ);
  qmc.setRange(QMC5883P_RANGE_8G);
  qmc.setOSR(QMC5883P_OSR_8);
  qmc.setSetResetMode(QMC5883P_SETRESET_ON);

  magnetometerConnected = true;

  Serial.println("[MAG] Configurazione:");
  Serial.printf("  Indirizzo: 0x%02X\n", magAddress);
  Serial.println("  Mode: Continuous");
  Serial.println("  ODR: 100Hz");
  Serial.println("  Range: 8G");
  Serial.println("  OSR: 8");

  delay(100);

  // Carica o esegui calibrazione
  if (!loadCalibration()) {
    Serial.println("[MAG] Nessuna calibrazione, avvio procedura...");
    calibrateMagnetometerGuided();
  }

  // Test lettura
  int16_t tx, ty, tz;
  if (qmc.getRawMagnetic(&tx, &ty, &tz)) {
    Serial.printf("[MAG] Test: X=%d Y=%d Z=%d\n", tx, ty, tz);
  }

  updateMagnetometer();

  Serial.println("\n[MAG] Magnetometro PRONTO!");
  Serial.printf("[MAG] Heading: %.1f° (%s)\n", magnetometerHeading, getCardinalDirection(magnetometerHeading));
  Serial.println("========================================\n");
}

// ================== AGGIORNAMENTO MAGNETOMETRO ==================
void updateMagnetometer() {
  if (!magnetometerConnected) return;

  // Aspetta dati pronti
  if (!qmc.isDataReady()) return;

  int16_t rawX, rawY, rawZ;
  if (!qmc.getRawMagnetic(&rawX, &rawY, &rawZ)) return;

  // Applica calibrazione
  float calX = (rawX - magOffsetX) * magScaleX;
  float calY = (rawY - magOffsetY) * magScaleY;
  float calZ = (rawZ - magOffsetZ) * magScaleZ;

  magX = (int16_t)calX;
  magY = (int16_t)calY;
  magZ = (int16_t)calZ;

  // Calcola heading (azimuth)
  float heading = atan2(calY, calX) * 180.0 / PI;

  // Aggiungi declinazione magnetica
  heading += magneticDeclination;

  // Normalizza 0-360
  if (heading < 0) heading += 360;
  if (heading >= 360) heading -= 360;

  magnetometerHeading = heading;

  // Debug ogni 2 secondi
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    lastDebug = millis();
    Serial.printf("[MAG] Raw:%d,%d,%d Cal:%d,%d,%d -> %.1f° %s\n",
                  rawX, rawY, rawZ, magX, magY, magZ,
                  magnetometerHeading, getCardinalDirection(magnetometerHeading));
  }
}

// ================== UTILITY ==================
void checkMagnetometerSerial() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'C' || c == 'c') {
      if (magnetometerConnected) {
        Serial.println("[MAG] Ricalibrrazione...");
        calibrateMagnetometerGuided();
      }
    }
  }
}

void clearMagCalibration() {
  magPrefs.begin("magnetometer", false);
  magPrefs.clear();
  magPrefs.end();
  magCalibrated = false;
  magOffsetX = magOffsetY = magOffsetZ = 0;
  magScaleX = magScaleY = magScaleZ = 1.0;
  Serial.println("[MAG] Calibrazione CANCELLATA!");
}

void printMagnetometerStatus() {
  Serial.println("\n=== Stato Magnetometro ===");
  Serial.printf("Connesso: %s\n", magnetometerConnected ? "SI" : "NO");
  Serial.printf("Indirizzo: 0x%02X\n", magAddress);
  Serial.printf("Calibrato: %s\n", magCalibrated ? "SI" : "NO");
  if (magnetometerConnected) {
    Serial.printf("X: %d, Y: %d, Z: %d\n", magX, magY, magZ);
    Serial.printf("Heading: %.1f° (%s)\n", magnetometerHeading, getCardinalDirection(magnetometerHeading));
  }
  Serial.println("==========================\n");
}
