#include "Hash.h"
#include "utility/trezor/hmac.h"
#include "utility/trezor/ripemd160.h"

#if USE_STD_STRING
using std::string;
#define String string
#endif

// generic funtcions for single line hash
static size_t hashData(HashAlgorithm * algo, const uint8_t * data, size_t len, uint8_t * hash){
    algo->begin();
    algo->write(data, len);
    return algo->end(hash);
}

#if USE_ARDUINO_STRING || USE_STD_STRING
static size_t hashString(HashAlgorithm * algo, const String s, uint8_t * hash){
    return hashData(algo, (const uint8_t *)s.c_str(), s.length(), hash);
}
#endif

/************************* RIPEMD-160 *************************/

int rmd160(const uint8_t * data, size_t len, uint8_t hash[20]){
    RMD160 rmd;
    return hashData(&rmd, data, len, hash);
}
int rmd160(const char * data, size_t len, uint8_t hash[20]){
    return rmd160((uint8_t*)data, len, hash);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int rmd160(const String data, uint8_t hash[20]){
    RMD160 rmd;
    return hashString(&rmd, data, hash);
}
#endif

void RMD160::begin(){
    ripemd160_Init(&ctx);
};
size_t RMD160::write(const uint8_t * data, size_t len){
    ripemd160_Update(&ctx, data, len);
    return len;
}
size_t RMD160::write(uint8_t b){
    uint8_t arr[1] = { b };
    ripemd160_Update(&ctx, arr, 1);
    return 1;
}
size_t RMD160::end(uint8_t hash[20]){
    ripemd160_Final(&ctx, hash);
    return 20;
}

/************************** SHA-256 **************************/

int sha256(const uint8_t * data, size_t len, uint8_t hash[32]){
    SHA256 sha;
    return hashData(&sha, data, len, hash);
}
int sha256(const char * data, size_t len, uint8_t hash[32]){
    return sha256((uint8_t*)data, len, hash);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int sha256(const String data, uint8_t hash[32]){
    SHA256 sha;
    return hashString(&sha, data, hash);
}
#endif

int sha256Hmac(const uint8_t * key, size_t keyLen, const uint8_t * data, size_t dataLen, uint8_t hash[32]){
    ubtc_hmac_sha256(key, keyLen, data, dataLen, hash);
    return 32;
}

void SHA256::begin(){
    sha256_Init(&ctx.ctx);
};
void SHA256::beginHMAC(const uint8_t * key, size_t keySize){
    ubtc_hmac_sha256_Init(&ctx, key, keySize);
}
size_t SHA256::write(const uint8_t * data, size_t len){
    sha256_Update(&ctx.ctx, data, len);
    return len;
}
size_t SHA256::write(uint8_t b){
    uint8_t arr[1] = { b };
    sha256_Update(&ctx.ctx, arr, 1);
    return 1;
}
size_t SHA256::end(uint8_t hash[32]){
    sha256_Final(&ctx.ctx, hash);
    return 32;
}
size_t SHA256::endHMAC(uint8_t hmac[32]){
    ubtc_hmac_sha256_Final(&ctx, hmac);
    return 32;
}

/************************* Hash-160 **************************/
/******************** rmd160( sha256( m ) ) ******************/

int hash160(const uint8_t * data, size_t len, uint8_t hash[20]){
    Hash160 h160;
    return hashData(&h160, data, len, hash);
}
int hash160(const char * data, size_t len, uint8_t hash[20]){
    return hash160((uint8_t*)data, len, hash);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int hash160(const String data, uint8_t hash[20]){
    Hash160 h160;
    return hashString(&h160, data, hash);
}
#endif

size_t Hash160::end(uint8_t hash[20]){
    uint8_t h[32];
    sha256_Final(&ctx.ctx, h);
    rmd160(h, 32, hash);
    return 20;
}

/********************** Double SHA-256 ***********************/
/******************** sha256( sha256( m ) ) ******************/

int doubleSha(const uint8_t * data, size_t len, uint8_t hash[32]){
    DoubleSha sha;
    return hashData(&sha, data, len, hash);
}
int doubleSha(const char * data, size_t len, uint8_t hash[32]){
    return doubleSha((uint8_t*)data, len, hash);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int doubleSha(const String data, uint8_t hash[32]){
    DoubleSha sha;
    return hashString(&sha, data, hash);
}
#endif

size_t DoubleSha::end(uint8_t hash[32]){
    uint8_t h[32];
    sha256_Final(&ctx.ctx, h);
    sha256(h, 32, hash);
    return 32;
}

/************************** SHA-512 **************************/

int sha512(const uint8_t * data, size_t len, uint8_t hash[64]){
    SHA512 sha;
    return hashData(&sha, data, len, hash);
}
int sha512(const char * data, size_t len, uint8_t hash[64]){
    return sha512((uint8_t*)data, len, hash);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int sha512(const String data, uint8_t hash[64]){
    SHA512 sha;
    return hashString(&sha, data, hash);
}
#endif

void SHA512::begin(){
    sha512_Init(&ctx.ctx);
};
void SHA512::beginHMAC(const uint8_t * key, size_t keySize){
    ubtc_hmac_sha512_Init(&ctx, key, keySize);
}
size_t SHA512::write(const uint8_t * data, size_t len){
    sha512_Update(&ctx.ctx, data, len);
    return len;
}
size_t SHA512::write(uint8_t b){
    uint8_t arr[1] = { b };
    sha512_Update(&ctx.ctx, arr, 1);
    return 1;
}
size_t SHA512::end(uint8_t hash[64]){
    sha512_Final(&ctx.ctx, hash);
    return 64;
}
size_t SHA512::endHMAC(uint8_t hmac[64]){
    ubtc_hmac_sha512_Final(&ctx, hmac);
    return 64;
}

int sha512Hmac(const uint8_t * key, size_t keyLen, const uint8_t * data, size_t dataLen, uint8_t hash[64]){
    ubtc_hmac_sha512(key, keyLen, data, dataLen, hash);
    return 64;
}
