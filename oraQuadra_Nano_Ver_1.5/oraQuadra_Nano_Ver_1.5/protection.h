
#ifndef PROTECTION_H
#define PROTECTION_H

void initProtection();

void printProtectedTitle();
void printProtectedBy();
void printProtectedAuthor();
void printAuthorPaolo();
void printAuthorDavide();
void printAuthorAlessandro();

bool distributedCheck1();
bool distributedCheck2();
bool distributedCheck3();
bool distributedCheck4();
bool distributedCheck5(uint32_t seed);

void showCreditsError(uint8_t errorCode);

extern uint8_t verificationFailures;
extern bool creditsVerified;

#define protectionFailCount verificationFailures
#define protectionFailed(code) showCreditsError(code)

#define PROTECTION_CHECK_SILENT() \
  do { \
    if (!distributedCheck1()) { \
      verificationFailures++; \
      if (verificationFailures >= 5) showCreditsError(0xD1); \
    } \
  } while(0)


#define PROTECTION_CHECK_RANDOM(x) \
  do { \
    if (!distributedCheck5(x)) { \
      verificationFailures++; \
      if (verificationFailures >= 5) showCreditsError(0xD5); \
    } \
  } while(0)

#endif 
