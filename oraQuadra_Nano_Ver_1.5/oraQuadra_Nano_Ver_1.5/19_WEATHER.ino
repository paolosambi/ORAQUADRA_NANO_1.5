// ================== OPEN-METEO API - METEO ESTERNO ==================
/*
 * Integrazione con Open-Meteo per ottenere:
 * - Temperatura esterna attuale
 * - Umidità esterna
 * - Condizioni meteo attuali
 * - Previsioni meteo per il giorno dopo
 *
 * API: https://open-meteo.com/ (GRATUITA, SENZA API KEY)
 * Geocoding: http://geocoding-api.open-meteo.com/
 */

#include <HTTPClient.h>
#include <ArduinoJson.h>

// ================== VARIABILI GLOBALI METEO ==================
float weatherTempOutdoor = -999.0;      // Temperatura esterna (°C)
float weatherHumOutdoor = -999.0;       // Umidità esterna (%)
String weatherDescription = "";          // Descrizione meteo (es. "Cielo sereno")
String weatherIcon = "";                 // Icona meteo (es. "01d")
int weatherCode = 0;                     // Codice meteo (WMO convertito in OpenWeatherMap)

// Dati aggiuntivi meteo attuale
float weatherWindSpeed = 0.0;           // Velocità vento (m/s)
int weatherWindDeg = 0;                  // Direzione vento (gradi 0-360)
float weatherPressure = 0.0;            // Pressione atmosferica (hPa)
float weatherFeelsLike = -999.0;        // Temperatura percepita (°C)
int weatherClouds = 0;                  // Copertura nuvolosa (%)
float weatherVisibility = 0.0;          // Visibilità (km)
long weatherSunrise = 0;                // Ora alba (timestamp)
long weatherSunset = 0;                 // Ora tramonto (timestamp)

// Previsioni giorno dopo
float weatherTempTomorrow = -999.0;     // Temperatura prevista domani (°C)
float weatherTempTomorrowMin = -999.0;  // Temperatura minima domani
float weatherTempTomorrowMax = -999.0;  // Temperatura massima domani
String weatherDescTomorrow = "";         // Descrizione meteo domani
String weatherIconTomorrow = "";         // Icona meteo domani
int weatherCodeTomorrow = 0;             // Codice meteo domani

bool weatherDataValid = false;           // Flag per indicare se i dati sono validi
// NOTA: lastWeatherUpdate e WEATHER_UPDATE_INTERVAL sono definiti nel file principale

// Coordinate geografiche (caricate da geocoding o impostate manualmente)
float weatherLatitude = 0.0;
float weatherLongitude = 0.0;
bool weatherCoordsValid = false;

// ================== FASE LUNARE ==================
float moonPhase = 0.0;                   // Fase lunare (0.0 - 1.0)
int moonPhaseIndex = 0;                  // Indice fase (0-7)
String moonPhaseName = "";               // Nome fase lunare
float moonIllumination = 0.0;            // Illuminazione luna (0-100%)
int moonAge = 0;                         // Età della luna in giorni (0-29)

// ================== MAPPATURA CODICI METEO WMO -> OPENWEATHERMAP ==================
// Include sistema multilingua
#include "language_config.h"
#include "language_strings.h"

// Converte codice meteo WMO (Open-Meteo) in codice OpenWeatherMap per compatibilità
int wmoToOpenWeatherCode(int wmoCode) {
  switch (wmoCode) {
    case 0:  return 800;   // Clear sky
    case 1:  return 801;   // Mainly clear
    case 2:  return 802;   // Partly cloudy
    case 3:  return 804;   // Overcast
    case 45: return 741;   // Fog
    case 48: return 741;   // Depositing rime fog
    case 51: return 300;   // Drizzle: Light
    case 53: return 301;   // Drizzle: Moderate
    case 55: return 302;   // Drizzle: Dense
    case 56: return 311;   // Freezing Drizzle: Light
    case 57: return 312;   // Freezing Drizzle: Dense
    case 61: return 500;   // Rain: Slight
    case 63: return 501;   // Rain: Moderate
    case 65: return 502;   // Rain: Heavy
    case 66: return 511;   // Freezing Rain: Light
    case 67: return 511;   // Freezing Rain: Heavy
    case 71: return 600;   // Snow fall: Slight
    case 73: return 601;   // Snow fall: Moderate
    case 75: return 602;   // Snow fall: Heavy
    case 77: return 600;   // Snow grains
    case 80: return 520;   // Rain showers: Slight
    case 81: return 521;   // Rain showers: Moderate
    case 82: return 522;   // Rain showers: Violent
    case 85: return 620;   // Snow showers: Slight
    case 86: return 621;   // Snow showers: Heavy
    case 95: return 200;   // Thunderstorm: Slight/moderate
    case 96: return 201;   // Thunderstorm with slight hail
    case 99: return 202;   // Thunderstorm with heavy hail
    default: return 800;   // Default: clear
  }
}

// Converte codice WMO in icona meteo stile OpenWeatherMap
String wmoToWeatherIcon(int wmoCode, bool isDay) {
  String suffix = isDay ? "d" : "n";
  switch (wmoCode) {
    case 0:  return "01" + suffix;  // Clear
    case 1:  return "02" + suffix;  // Mainly clear
    case 2:  return "03" + suffix;  // Partly cloudy
    case 3:  return "04" + suffix;  // Overcast
    case 45:
    case 48: return "50" + suffix;  // Fog
    case 51:
    case 53:
    case 55:
    case 56:
    case 57: return "09" + suffix;  // Drizzle
    case 61:
    case 63:
    case 80:
    case 81: return "10" + suffix;  // Rain
    case 65:
    case 66:
    case 67:
    case 82: return "09" + suffix;  // Heavy rain
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86: return "13" + suffix;  // Snow
    case 95:
    case 96:
    case 99: return "11" + suffix;  // Thunderstorm
    default: return "01" + suffix;
  }
}

// Converte codice meteo in descrizione (multilingua)
const char* getWeatherDescriptionIT(int code) {
  // Usa la funzione helper di language_strings.h che gestisce automaticamente la lingua
  return getWeatherString(code);
}

// ================== FUNZIONE URL ENCODE ==================
String weatherUrlEncode(const String& str) {
  String encoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      char buf[4];
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

// ================== FUNZIONE GEOCODING ==================
// Converte nome città in coordinate lat/lon usando Open-Meteo Geocoding API
bool geocodeCity(const String& cityName) {
  if (cityName.length() < 2) {
    Serial.println("[WEATHER] Nome città troppo corto per geocoding");
    return false;
  }

  HTTPClient http;

  // URL encode del nome città
  String encodedCity = weatherUrlEncode(cityName);

  // URL API geocoding Open-Meteo
  String url = "http://geocoding-api.open-meteo.com/v1/search?name=";
  url += encodedCity;
  url += "&count=1&language=it&format=json";

  Serial.printf("[WEATHER] Geocoding città: %s\n", cityName.c_str());
  Serial.printf("[WEATHER] URL: %s\n", url.c_str());

  http.begin(url);
  http.setTimeout(15000);  // Timeout aumentato

  int httpCode = http.GET();
  Serial.printf("[WEATHER] HTTP Response: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK || httpCode == 200) {
    String payload = http.getString();
    Serial.printf("[WEATHER] Payload ricevuto: %d bytes\n", payload.length());

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.printf("[WEATHER] Errore parsing geocoding JSON: %s\n", error.c_str());
      Serial.println(payload.substring(0, 200));  // Debug: mostra primi 200 char
      http.end();
      return false;
    }

    // Verifica che ci siano risultati
    if (!doc.containsKey("results") || doc["results"].size() == 0) {
      Serial.println("[WEATHER] Nessun risultato geocoding trovato");
      Serial.println(payload);  // Debug: mostra risposta
      http.end();
      return false;
    }

    // Estrai coordinate dal primo risultato
    weatherLatitude = doc["results"][0]["latitude"].as<float>();
    weatherLongitude = doc["results"][0]["longitude"].as<float>();
    String foundCity = doc["results"][0]["name"].as<String>();
    String country = doc["results"][0]["country"].as<String>();

    weatherCoordsValid = true;

    Serial.println("[WEATHER] === GEOCODING OK ===");
    Serial.printf("[WEATHER] Città trovata: %s, %s\n", foundCity.c_str(), country.c_str());
    Serial.printf("[WEATHER] Coordinate: %.4f, %.4f\n", weatherLatitude, weatherLongitude);

    http.end();
    return true;

  } else {
    Serial.printf("[WEATHER] Errore HTTP geocoding: %d\n", httpCode);
    if (httpCode < 0) {
      Serial.println("[WEATHER] Errore connessione - verificare WiFi e DNS");
    }
    http.end();
    return false;
  }
}

// ================== FUNZIONE AGGIORNAMENTO METEO ATTUALE ==================
bool updateCurrentWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WEATHER] WiFi non connesso, skip aggiornamento meteo");
    return false;
  }

  // Usa le variabili configurabili da web (definite in 26_WEBSERVER_SETTINGS.ino)
  extern String apiOpenWeatherCity;  // Nome città (usato per geocoding)

  // Verifica che la città sia configurata
  if (apiOpenWeatherCity.length() < 2) {
    Serial.println("[WEATHER] Città meteo non configurata! Inserirla da /settings");
    return false;
  }

  // Se non abbiamo le coordinate, esegui geocoding
  if (!weatherCoordsValid) {
    if (!geocodeCity(apiOpenWeatherCity)) {
      Serial.println("[WEATHER] Geocoding fallito, impossibile ottenere meteo");
      return false;
    }
  }

  HTTPClient http;

  // URL API Open-Meteo per meteo attuale
  // Richiediamo: temperatura, umidità, pressione, vento, nuvolosità, codice meteo
  String url = "http://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(weatherLatitude, 4);
  url += "&longitude=" + String(weatherLongitude, 4);
  url += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,";
  url += "surface_pressure,wind_speed_10m,wind_direction_10m,weather_code,cloud_cover";
  url += "&daily=sunrise,sunset";
  url += "&timezone=auto";

  Serial.println("[WEATHER] Richiesta meteo attuale Open-Meteo...");
  Serial.printf("[WEATHER] Coordinate: %.4f, %.4f\n", weatherLatitude, weatherLongitude);

  http.begin(url);
  http.setTimeout(10000);  // Timeout 10 secondi

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("[WEATHER] Risposta ricevuta");

    // Parse JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("[WEATHER] Errore parsing JSON: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }

    // Estrai dati attuali
    JsonObject current = doc["current"];

    weatherTempOutdoor = current["temperature_2m"].as<float>();
    weatherHumOutdoor = current["relative_humidity_2m"].as<float>();
    weatherFeelsLike = current["apparent_temperature"].as<float>();
    weatherPressure = current["surface_pressure"].as<float>();
    weatherWindSpeed = current["wind_speed_10m"].as<float>() / 3.6;  // km/h -> m/s
    weatherWindDeg = current["wind_direction_10m"].as<int>();
    weatherClouds = current["cloud_cover"].as<int>();

    // Codice meteo WMO -> OpenWeatherMap per compatibilità
    int wmoCode = current["weather_code"].as<int>();
    weatherCode = wmoToOpenWeatherCode(wmoCode);
    weatherDescription = getWeatherDescriptionIT(weatherCode);

    // Determina se è giorno o notte per l'icona
    bool isDay = true;  // Default giorno
    if (doc.containsKey("daily")) {
      // Estrai alba e tramonto di oggi
      String sunriseStr = doc["daily"]["sunrise"][0].as<String>();
      String sunsetStr = doc["daily"]["sunset"][0].as<String>();

      // Parse semplificato dell'ora (formato ISO: YYYY-MM-DDTHH:MM)
      int sunriseHour = sunriseStr.substring(11, 13).toInt();
      int sunriseMin = sunriseStr.substring(14, 16).toInt();
      int sunsetHour = sunsetStr.substring(11, 13).toInt();
      int sunsetMin = sunsetStr.substring(14, 16).toInt();
      int currentHourNow = myTZ.hour();

      isDay = (currentHourNow >= sunriseHour && currentHourNow < sunsetHour);

      // Converte alba/tramonto in timestamp per la visualizzazione SOLE
      // Usa la data di oggi con l'ora estratta
      struct tm tmSunrise = {0};
      struct tm tmSunset = {0};

      tmSunrise.tm_year = myTZ.year() - 1900;
      tmSunrise.tm_mon = myTZ.month() - 1;
      tmSunrise.tm_mday = myTZ.day();
      tmSunrise.tm_hour = sunriseHour;
      tmSunrise.tm_min = sunriseMin;
      tmSunrise.tm_sec = 0;

      tmSunset.tm_year = myTZ.year() - 1900;
      tmSunset.tm_mon = myTZ.month() - 1;
      tmSunset.tm_mday = myTZ.day();
      tmSunset.tm_hour = sunsetHour;
      tmSunset.tm_min = sunsetMin;
      tmSunset.tm_sec = 0;

      weatherSunrise = mktime(&tmSunrise);
      weatherSunset = mktime(&tmSunset);

      Serial.printf("[WEATHER] Alba: %02d:%02d (ts: %ld)\n", sunriseHour, sunriseMin, weatherSunrise);
      Serial.printf("[WEATHER] Tramonto: %02d:%02d (ts: %ld)\n", sunsetHour, sunsetMin, weatherSunset);
    }

    weatherIcon = wmoToWeatherIcon(wmoCode, isDay);

    // Visibilità non disponibile in Open-Meteo base, stima dalla nuvolosità
    weatherVisibility = (100 - weatherClouds) / 10.0 + 5.0;  // 5-15 km stima

    Serial.println("[WEATHER] === METEO ATTUALE (Open-Meteo) ===");
    Serial.printf("[WEATHER] Temperatura: %.1f°C (percepita: %.1f°C)\n", weatherTempOutdoor, weatherFeelsLike);
    Serial.printf("[WEATHER] Umidità: %.0f%%\n", weatherHumOutdoor);
    Serial.printf("[WEATHER] Pressione: %.0f hPa\n", weatherPressure);
    Serial.printf("[WEATHER] Vento: %.1f m/s da %d°\n", weatherWindSpeed, weatherWindDeg);
    Serial.printf("[WEATHER] Nuvolosità: %d%%\n", weatherClouds);
    Serial.printf("[WEATHER] Condizioni: %s (WMO: %d -> OWM: %d)\n", weatherDescription.c_str(), wmoCode, weatherCode);
    Serial.println("[WEATHER] ====================");

    weatherDataValid = true;
    http.end();
    return true;

  } else {
    Serial.printf("[WEATHER] Errore HTTP: %d\n", httpCode);
    http.end();
    return false;
  }
}

// ================== FUNZIONE AGGIORNAMENTO PREVISIONI ==================
bool updateWeatherForecast() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WEATHER] WiFi non connesso, skip previsioni");
    return false;
  }

  if (!weatherCoordsValid) {
    Serial.println("[WEATHER] Coordinate non valide, skip previsioni");
    return false;
  }

  HTTPClient http;

  // URL API Open-Meteo per previsioni giornaliere
  String url = "http://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(weatherLatitude, 4);
  url += "&longitude=" + String(weatherLongitude, 4);
  url += "&daily=temperature_2m_max,temperature_2m_min,weather_code";
  url += "&timezone=auto";
  url += "&forecast_days=2";  // Oggi + domani

  Serial.println("[WEATHER] Richiesta previsioni Open-Meteo...");

  http.begin(url);
  http.setTimeout(15000);  // Timeout 15 secondi

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("[WEATHER] Previsioni ricevute");

    // Parse JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("[WEATHER] Errore parsing JSON previsioni: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }

    // Estrai dati previsione domani (indice 1)
    JsonObject daily = doc["daily"];

    if (daily["temperature_2m_max"].size() > 1) {
      weatherTempTomorrowMax = daily["temperature_2m_max"][1].as<float>();
      weatherTempTomorrowMin = daily["temperature_2m_min"][1].as<float>();
      weatherTempTomorrow = (weatherTempTomorrowMax + weatherTempTomorrowMin) / 2.0;

      int wmoCodeTomorrow = daily["weather_code"][1].as<int>();
      weatherCodeTomorrow = wmoToOpenWeatherCode(wmoCodeTomorrow);
      weatherDescTomorrow = getWeatherDescriptionIT(weatherCodeTomorrow);
      weatherIconTomorrow = wmoToWeatherIcon(wmoCodeTomorrow, true);  // Assume giorno

      Serial.println("[WEATHER] === PREVISIONI DOMANI ===");
      Serial.printf("[WEATHER] Temperatura: %.1f°C (min: %.1f, max: %.1f)\n",
                    weatherTempTomorrow, weatherTempTomorrowMin, weatherTempTomorrowMax);
      Serial.printf("[WEATHER] Condizioni: %s\n", weatherDescTomorrow.c_str());
      Serial.println("[WEATHER] ===========================");
    }

    http.end();
    return true;

  } else {
    Serial.printf("[WEATHER] Errore HTTP previsioni: %d\n", httpCode);
    http.end();
    return false;
  }
}

// ================== FUNZIONE AGGIORNAMENTO COMPLETO ==================
void updateWeatherData() {
  unsigned long currentMillis = millis();

  // Aggiorna solo ogni WEATHER_UPDATE_INTERVAL
  if (currentMillis - lastWeatherUpdate < WEATHER_UPDATE_INTERVAL && lastWeatherUpdate > 0) {
    return;
  }

  Serial.println("\n[WEATHER] ========== AGGIORNAMENTO METEO (Open-Meteo) ==========");

  // Aggiorna meteo attuale
  if (updateCurrentWeather()) {
    // Se meteo attuale OK, aggiorna anche previsioni
    delay(500);  // Breve pausa tra le chiamate API
    updateWeatherForecast();
  }

  // Aggiorna sempre la fase lunare (non richiede API)
  updateMoonPhase();

  lastWeatherUpdate = currentMillis;
  Serial.println("[WEATHER] ==========================================\n");
}

// ================== FUNZIONE INIT METEO ==================
void initWeather() {
  extern String apiOpenWeatherCity;
  Serial.println("[WEATHER] Inizializzazione modulo meteo Open-Meteo...");
  Serial.printf("[WEATHER] Città: %s\n", apiOpenWeatherCity.c_str());

  // Reset coordinate per forzare nuovo geocoding
  weatherCoordsValid = false;

  // Prima lettura immediata
  updateWeatherData();
}

// ================== FUNZIONE RESET COORDINATE ==================
// Chiamare quando cambia la città per forzare nuovo geocoding
void resetWeatherCoords() {
  weatherCoordsValid = false;
  weatherLatitude = 0.0;
  weatherLongitude = 0.0;
  Serial.println("[WEATHER] Coordinate resettate, verrà eseguito nuovo geocoding");
}

// ================== CALCOLO FASE LUNARE ==================
/*
 * Algoritmo per calcolare la fase lunare basato sul ciclo sinodico
 * Il ciclo lunare dura circa 29.53 giorni
 *
 * Fasi:
 * 0 = Luna Nuova (0%)
 * 1 = Luna Crescente (1-24%)
 * 2 = Primo Quarto (25%)
 * 3 = Gibbosa Crescente (26-49%)
 * 4 = Luna Piena (50%)
 * 5 = Gibbosa Calante (51-74%)
 * 6 = Ultimo Quarto (75%)
 * 7 = Luna Calante (76-99%)
 */

// Nomi delle fasi lunari (italiano)
const char* MOON_PHASE_NAMES[] = {
  "Luna Nuova",
  "Crescente",
  "Primo Quarto",
  "Gibbosa Cresc.",
  "Luna Piena",
  "Gibbosa Cal.",
  "Ultimo Quarto",
  "Calante"
};

// Calcola la fase lunare per una data specifica
// Restituisce un valore da 0.0 a 1.0 dove:
// 0.0 = Luna Nuova
// 0.25 = Primo Quarto
// 0.5 = Luna Piena
// 0.75 = Ultimo Quarto
float calculateMoonPhase(int year, int month, int day) {
  // Algoritmo di Conway per la fase lunare
  // Riferimento: Luna nuova il 6 Gennaio 2000

  // Converti la data in numero di giorni dal riferimento
  // Usando una formula semplificata

  int r = year % 100;
  r %= 19;
  if (r > 9) r -= 19;
  r = ((r * 11) % 30) + month + day;
  if (month < 3) r += 2;

  // Correzione per anni bisestili
  if (year < 2000) {
    r -= 4;
  } else {
    r -= 8.3;
  }

  r = ((int)(r + 0.5)) % 30;
  if (r < 0) r += 30;

  // r è ora l'età della luna in giorni (0-29)
  moonAge = r;

  // Converti in fase (0.0 - 1.0)
  float phase = (float)r / 29.53;
  if (phase > 1.0) phase = phase - 1.0;
  if (phase < 0.0) phase = phase + 1.0;

  return phase;
}

// Calcola l'illuminazione della luna (0-100%)
float calculateMoonIllumination(float phase) {
  // L'illuminazione segue una curva sinusoidale
  // Luna nuova = 0%, Luna piena = 100%
  float illum = (1.0 - cos(phase * 2.0 * PI)) / 2.0 * 100.0;
  return illum;
}

// Determina l'indice della fase (0-7)
int getMoonPhaseIndex(float phase) {
  // Dividi il ciclo in 8 fasi
  // Ogni fase copre circa 3.7 giorni

  if (phase < 0.0625) return 0;       // Luna Nuova
  else if (phase < 0.1875) return 1;  // Crescente
  else if (phase < 0.3125) return 2;  // Primo Quarto
  else if (phase < 0.4375) return 3;  // Gibbosa Crescente
  else if (phase < 0.5625) return 4;  // Luna Piena
  else if (phase < 0.6875) return 5;  // Gibbosa Calante
  else if (phase < 0.8125) return 6;  // Ultimo Quarto
  else if (phase < 0.9375) return 7;  // Calante
  else return 0;                       // Luna Nuova (fine ciclo)
}

// Aggiorna tutti i dati della fase lunare
void updateMoonPhase() {
  // Ottieni data corrente
  int year = myTZ.year();
  int month = myTZ.month();
  int day = myTZ.day();

  // Calcola fase lunare
  moonPhase = calculateMoonPhase(year, month, day);

  // Calcola illuminazione
  moonIllumination = calculateMoonIllumination(moonPhase);

  // Determina indice fase
  moonPhaseIndex = getMoonPhaseIndex(moonPhase);

  // Nome fase
  moonPhaseName = MOON_PHASE_NAMES[moonPhaseIndex];

  Serial.println("[MOON] === FASE LUNARE ===");
  Serial.printf("[MOON] Data: %d/%d/%d\n", day, month, year);
  Serial.printf("[MOON] Fase: %.2f (%.0f%% illuminata)\n", moonPhase, moonIllumination);
  Serial.printf("[MOON] Età: %d giorni\n", moonAge);
  Serial.printf("[MOON] Nome: %s\n", moonPhaseName.c_str());
  Serial.println("[MOON] ===================");
}
