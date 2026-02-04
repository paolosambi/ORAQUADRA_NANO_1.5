// ============================================================================
// PROTEZIONE CREDITI - Header File
// ============================================================================
// Questo file contiene le dichiarazioni per il sistema di protezione crediti.
// Include questo file nel main .ino per avere accesso alle funzioni.
// ============================================================================

#ifndef PROTECTION_H
#define PROTECTION_H

// ============================================================================
// FUNZIONI PUBBLICHE (implementate in 0A_PROTECTION.ino)
// ============================================================================

// Inizializza e verifica i crediti all'avvio
void initProtection();

// Stampa titolo e crediti protetti sul display
void printProtectedTitle();
void printProtectedBy();
void printProtectedAuthor();
void printAuthorPaolo();
void printAuthorDavide();
void printAuthorAlessandro();

// Controlli distribuiti (chiamati periodicamente)
bool distributedCheck1();
bool distributedCheck2();
bool distributedCheck3();
bool distributedCheck4();
bool distributedCheck5(uint32_t seed);

// Mostra errore crediti (blocca il sistema)
void showCreditsError(uint8_t errorCode);

// ============================================================================
// VARIABILI GLOBALI (definite in 0A_PROTECTION.ino)
// ============================================================================
extern uint8_t verificationFailures;
extern bool creditsVerified;

// Alias per compatibilita'
#define protectionFailCount verificationFailures
#define protectionFailed(code) showCreditsError(code)

// ============================================================================
// MACRO DI CONTROLLO
// ============================================================================

// Controllo silenzioso periodico
#define PROTECTION_CHECK_SILENT() \
  do { \
    if (!distributedCheck1()) { \
      verificationFailures++; \
      if (verificationFailures >= 5) showCreditsError(0xD1); \
    } \
  } while(0)

// Controllo casuale basato su seed
#define PROTECTION_CHECK_RANDOM(x) \
  do { \
    if (!distributedCheck5(x)) { \
      verificationFailures++; \
      if (verificationFailures >= 5) showCreditsError(0xD5); \
    } \
  } while(0)

#endif // PROTECTION_H
