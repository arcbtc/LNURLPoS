/** @file Hash.h
 *  \brief All hashing functions and classes live here
 */
#ifndef __HASH_H__18NLNNCSJ2
#define __HASH_H__18NLNNCSJ2

#include "uBitcoin_conf.h"
#include "BaseClasses.h"
#include <stdint.h>
#include <string.h>
#include "utility/trezor/sha2.h"
#include "utility/trezor/ripemd160.h"
#include "utility/trezor/hmac.h"

/** \brief Abstract hashing class */
class HashAlgorithm : public SerializeStream{
public:
    size_t available(){ return 100; };
	void begin(){};
    virtual size_t write(const uint8_t * data, size_t len) = 0;
    virtual size_t write(uint8_t b) = 0;
    virtual size_t end(uint8_t * hash) = 0;
};

/************************* RIPEMD-160 *************************/

/** \brief ripemd-160 one-line hashing function */
int rmd160(const uint8_t * data, size_t len, uint8_t hash[20]);
int rmd160(const char * data, size_t len, uint8_t hash[20]);
#if USE_ARDUINO_STRING
int rmd160(const String data, uint8_t hash[20]);
#endif
#if USE_STD_STRING
int rmd160(const std::string data, uint8_t hash[20]);
#endif

class RMD160 : public HashAlgorithm{
public:
    RMD160(){ begin(); };
    void begin();
    size_t write(const uint8_t * data, size_t len);
    size_t write(uint8_t b);
    size_t end(uint8_t hash[20]);
protected:
    RIPEMD160_CTX ctx;
};

/************************** SHA-256 **************************/

/** \brief sha256 one-line hashing function → 32 bytes output */
int sha256(const uint8_t * data, size_t len, uint8_t hash[32]);
int sha256(const char * data, size_t len, uint8_t hash[32]);
#if USE_ARDUINO_STRING
int sha256(const String data, uint8_t hash[32]);
#endif
#if USE_STD_STRING
int sha256(const std::string data, uint8_t hash[32]);
#endif

int sha256Hmac(const uint8_t * key, size_t keyLen, const uint8_t * data, size_t dataLen, uint8_t hash[32]);

class SHA256 : public HashAlgorithm{
public:
    SHA256(){ begin(); };
    void begin();
    void beginHMAC(const uint8_t * key, size_t keySize);
    size_t write(const uint8_t * data, size_t len);
    size_t write(uint8_t b);
    size_t end(uint8_t hash[32]);
    size_t endHMAC(uint8_t hmac[32]);
protected:
    HMAC_SHA256_CTX ctx;
};

/************************* Hash-160 **************************/
/******************** rmd160( sha256( m ) ) ******************/

/** \brief rmd160(sha256(data)) → 20 bytes output */
int hash160(const uint8_t * data, size_t len, uint8_t hash[20]);
int hash160(const uint8_t * data, size_t len, uint8_t hash[20]);
int hash160(const char * data, size_t len, uint8_t hash[20]);
#if USE_ARDUINO_STRING
int hash160(const String data, uint8_t hash[20]);
#endif
#if USE_STD_STRING
int hash160(const std::string data, uint8_t hash[20]);
#endif

class Hash160 : public SHA256{
public:
    size_t end(uint8_t hash[20]);
};

/********************** Double SHA-256 ***********************/
/******************** sha256( sha256( m ) ) ******************/

/** \brief sha256(sha256(data)) → 32 bytes output */
int doubleSha(const uint8_t * data, size_t len, uint8_t hash[32]);
int doubleSha(const char * data, size_t len, uint8_t hash[32]);
#if USE_ARDUINO_STRING
int doubleSha(const String data, uint8_t hash[32]);
#endif
#if USE_STD_STRING
int doubleSha(const std::string data, uint8_t hash[32]);
#endif

class DoubleSha : public SHA256{
public:
    size_t end(uint8_t hash[32]);
};

/************************** SHA-512 **************************/

int sha512Hmac(const uint8_t * key, size_t keyLen, const uint8_t * data, size_t dataLen, uint8_t hash[64]);

int sha512(const uint8_t * data, size_t len, uint8_t hash[64]);
int sha512(const char * data, size_t len, uint8_t hash[64]);
#if USE_ARDUINO_STRING
int sha512(const String data, uint8_t hash[64]);
#endif
#if USE_STD_STRING
int sha512(const std::string data, uint8_t hash[64]);
#endif

class SHA512 : public HashAlgorithm{
public:
    SHA512(){ begin(); };
    void begin();
    void beginHMAC(const uint8_t * key, size_t keySize);
    size_t write(const uint8_t * data, size_t len);
    size_t write(uint8_t b);
    size_t end(uint8_t hash[64]);
    size_t endHMAC(uint8_t hmac[64]);
protected:
    HMAC_SHA512_CTX ctx;
};

#endif // __HASH_H__18NLNNCSJ2
