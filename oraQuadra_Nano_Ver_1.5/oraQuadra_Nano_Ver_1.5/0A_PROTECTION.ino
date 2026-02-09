
#define CREDITS_XOR_KEY 0x5A

const uint8_t CREDITS_TITLE[] PROGMEM = {
  0x15, 0x08, 0x1B, 0x0B, 0x0F, 0x1B, 0x1E, 0x08, 0x1B, 0x7A, 0x14, 0x1B, 0x14, 0x15, 0x00
};

const uint8_t CREDITS_BY[] PROGMEM = {
  0x18, 0x03, 0x00
};

const uint8_t CREDITS_TEAM[] PROGMEM = {
  0x09, 0x2F, 0x28, 0x2C, 0x33, 0x2C, 0x3B, 0x36, 0x12, 0x3B, 0x39, 0x31, 0x33, 0x34, 0x3D, 0x00
};

const uint8_t CREDITS_PAOLO[] PROGMEM = {
  0x0A, 0x3B, 0x35, 0x36, 0x35, 0x7A, 0x09, 0x3B, 0x37, 0x38, 0x33, 0x34, 0x3F, 0x36, 0x36, 0x35, 0x00
};
const uint8_t CREDITS_DAVIDE[] PROGMEM = {
  0x1E, 0x3B, 0x2C, 0x33, 0x3E, 0x3F, 0x7A, 0x1D, 0x3B, 0x2E, 0x2E, 0x33, 0x00
};

const uint8_t CREDITS_ALESSANDRO[] PROGMEM = {
  0x1B, 0x36, 0x3F, 0x29, 0x29, 0x3B, 0x34, 0x3E, 0x28, 0x35, 0x7A, 0x09, 0x2A, 0x3B, 0x3D, 0x34, 0x35, 0x36, 0x3F, 0x2E, 0x2E, 0x33, 0x00
};

bool creditsVerified = false;              
uint8_t verificationFailures = 0;          
static uint32_t lastVerificationTime = 0;  
static char decodedString[64];             

static const uint32_t crc32Table[256] PROGMEM = {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
  0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
  0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
  0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
  0xDBBBB9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
  0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
  0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
  0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
  0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
  0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
  0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
  0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
  0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
  0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
  0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
  0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
  0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
  0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
  0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
  0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAE00A1B6, 0xD9099B29,
  0x40036394, 0x37006602, 0xAE0D7FA1, 0xD9007637, 0x4009278D, 0x3700171B,
  0xA60D1798, 0xD10A270E, 0x480376B4, 0x3F044622, 0xA1006681, 0xD6075617,
  0x4F0005AD, 0x3807353B, 0xA80A06AA, 0xDF0D363C, 0x46046786, 0x31035710,
  0xAF0FC0B3, 0xD808F025, 0x4101A19F, 0x36069109
};

static uint32_t calculateCreditsCRC(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    uint8_t b = pgm_read_byte(&data[i]);
    uint8_t tableIndex = (crc ^ b) & 0xFF;
    crc = pgm_read_dword(&crc32Table[tableIndex]) ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFF;
}

static const char* decodeCredits(const uint8_t* encoded) {
  int i = 0;
  while (i < 63) {
    uint8_t b = pgm_read_byte(&encoded[i]);
    if (b == 0) break;
    decodedString[i] = b ^ CREDITS_XOR_KEY;
    i++;
  }
  decodedString[i] = '\0';
  return decodedString;
}

static bool verifyTitle() {
  const char* title = decodeCredits(CREDITS_TITLE);

  if (strlen(title) != 14) return false;
  if (title[0] != 'O') return false;
  if (title[1] != 'R') return false;
  if (title[2] != 'A') return false;
  if (title[3] != 'Q') return false;
  if (title[4] != 'U') return false;
  if (title[5] != 'A') return false;
  if (title[6] != 'D') return false;
  if (title[7] != 'R') return false;
  if (title[8] != 'A') return false;
  if (title[9] != ' ') return false;
  if (title[10] != 'N') return false;
  if (title[11] != 'A') return false;
  if (title[12] != 'N') return false;
  if (title[13] != 'O') return false;

  return true;
}

static bool verifyBy() {
  const char* by = decodeCredits(CREDITS_BY);
  return (by[0] == 'B' && by[1] == 'Y' && by[2] == '\0');
}

static bool verifyLogo() {
  uint32_t checksum = 0;


  for (int i = 0; i < 480 * 480; i += 9973) {  
    uint16_t pixel = pgm_read_word(&sh_logo_480x480[i]);
    checksum = checksum * 31 + pixel;
  }


  checksum += pgm_read_word(&sh_logo_480x480[115440]);  // Centro
  checksum += pgm_read_word(&sh_logo_480x480[48100]);   // Alto
  checksum += pgm_read_word(&sh_logo_480x480[182780]);  // Basso

  return (checksum != 0);
}


static bool verifyAllCredits() {
  if (!verifyTitle()) {
    Serial.println("[CREDITS] Errore: Titolo modificato!");
    return false;
  }

  if (!verifyBy()) {
    Serial.println("[CREDITS] Errore: Attribuzione modificata!");
    return false;
  }

  if (!verifyLogo()) {
    Serial.println("[CREDITS] Errore: Logo modificato!");
    return false;
  }

  uint32_t combinedCRC = 0;
  combinedCRC ^= calculateCreditsCRC(CREDITS_TITLE, sizeof(CREDITS_TITLE));
  combinedCRC ^= calculateCreditsCRC(CREDITS_BY, sizeof(CREDITS_BY));
  combinedCRC ^= calculateCreditsCRC(CREDITS_TEAM, sizeof(CREDITS_TEAM));
  combinedCRC ^= calculateCreditsCRC(CREDITS_PAOLO, sizeof(CREDITS_PAOLO));
  combinedCRC ^= calculateCreditsCRC(CREDITS_DAVIDE, sizeof(CREDITS_DAVIDE));
  combinedCRC ^= calculateCreditsCRC(CREDITS_ALESSANDRO, sizeof(CREDITS_ALESSANDRO));

  if (combinedCRC == 0) {
    Serial.println("[CREDITS] Errore: Dati corrotti!");
    return false;
  }

  creditsVerified = true;
  return true;
}

void showCreditsError(uint8_t errorCode) {
  // Schermo rosso con messaggio chiaro
  gfx->fillScreen(0xF800);  // Rosso
  gfx->setTextColor(0xFFFF); // Bianco

  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setCursor(60, 120);
  gfx->println("CREDITI MODIFICATI");

  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setCursor(40, 180);
  gfx->println("I crediti degli autori originali");
  gfx->setCursor(40, 200);
  gfx->println("sono stati modificati o rimossi.");

  gfx->setCursor(40, 250);
  gfx->println("Questo progetto e' OPEN SOURCE ma");
  gfx->setCursor(40, 270);
  gfx->println("richiede di mantenere i crediti a:");

  gfx->setTextColor(0xFFE0); // Giallo
  gfx->setCursor(40, 310);
  gfx->println("Paolo Sambinello");
  gfx->setCursor(40, 330);
  gfx->println("Davide Gatti");
  gfx->setCursor(40, 350);
  gfx->println("Alessandro Spagnoletti");
  gfx->setCursor(40, 370);
  gfx->println("www.survivalhacking.it");

  gfx->setTextColor(0xFFFF);
  gfx->setCursor(40, 420);
  gfx->print("Codice errore: 0x");
  gfx->println(errorCode, HEX);

  gfx->setCursor(40, 450);
  gfx->println("Ripristina il firmware originale.");

  ledcWrite(PWM_CHANNEL, 200);

  Serial.println("================================================");
  Serial.println("ERRORE: I crediti degli autori sono stati modificati!");
  Serial.println("Questo progetto richiede di mantenere i crediti.");
  Serial.println("Ripristina il firmware originale da:");
  Serial.println("https://github.com/paolosambi/ORAQUADRA_NANO_1.5");
  Serial.println("================================================");


  while (1) {
    delay(1000);
    ledcWrite(PWM_CHANNEL, 50);
    delay(1000);
    ledcWrite(PWM_CHANNEL, 200);
  }
}

bool distributedCheck1() {
  if (!creditsVerified) return false;
  uint8_t firstByte = pgm_read_byte(&CREDITS_TITLE[0]);
  return ((firstByte ^ CREDITS_XOR_KEY) == 'O');
}

bool distributedCheck2() {
  if (!creditsVerified) return false;
  uint8_t len = 0;
  while (pgm_read_byte(&CREDITS_BY[len]) != 0 && len < 10) len++;
  return (len == 2);  // "BY" = 2 caratteri
}

bool distributedCheck3() {
  if (!creditsVerified) return false;
  return (verificationFailures < 3);
}

bool distributedCheck4() {

  if (millis() - lastVerificationTime > 60000) {
    lastVerificationTime = millis();
    if (!verifyTitle()) {
      verificationFailures++;
      return false;
    }
  }
  return true;
}

bool distributedCheck5(uint32_t seed) {
  if ((seed % 0x1337) == 0) {
    return verifyBy();
  }
  return true;
}

void initProtection() {
  Serial.println("[CREDITS] Verifica crediti autori...");

  creditsVerified = false;
  verificationFailures = 0;
  lastVerificationTime = millis();

  if (!verifyAllCredits()) {
    Serial.println("[CREDITS] VERIFICA FALLITA!");
    showCreditsError(0x01);
  }

  Serial.println("[CREDITS] Crediti verificati OK");
  Serial.println("[CREDITS] Progetto by: Paolo, Davide, Alessandro");
  Serial.println("[CREDITS] www.survivalhacking.it");
}

void printProtectedTitle() {
  const char* title = decodeCredits(CREDITS_TITLE);
  gfx->println(title);
}

void printProtectedBy() {
  const char* by = decodeCredits(CREDITS_BY);
  gfx->println(by);
}

void printProtectedAuthor() {
  const char* team = decodeCredits(CREDITS_TEAM);
  gfx->println(team);
}

void printAuthorPaolo() {
  const char* name = decodeCredits(CREDITS_PAOLO);
  gfx->println(name);
}

void printAuthorDavide() {
  const char* name = decodeCredits(CREDITS_DAVIDE);
  gfx->println(name);
}

void printAuthorAlessandro() {
  const char* name = decodeCredits(CREDITS_ALESSANDRO);
  gfx->println(name);
}
