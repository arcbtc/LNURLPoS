#include <stdint.h>
#include <string.h>

#include "Bitcoin.h"
#include "Hash.h"
#include "Conversion.h"
#include "utility/trezor/sha2.h"
#include "utility/segwit_addr.h"
#include "utility/trezor/bignum.h"
#include "utility/trezor/ecdsa.h"
#include "utility/trezor/secp256k1.h"
#include "utility/trezor/memzero.h"

#if USE_STD_STRING
using std::string;
#define String string
#endif

// ---------------------------------------------------------------- HDPrivateKey class

void HDPrivateKey::init(){
    reset();
    memzero(chainCode, 32);
    depth = 0;
    memzero(parentFingerprint, 4);
    childNumber = 0;
    type = UNKNOWN_TYPE;
    status = PARSING_DONE;
    pubKey.compressed = true;
}
HDPrivateKey::HDPrivateKey(void):PrivateKey(){
    init();
}
HDPrivateKey::HDPrivateKey(const uint8_t secret[32],
                           const uint8_t chain_code[32],
                           uint8_t key_depth,
                           const uint8_t parent_fingerprint_arr[4],
                           uint32_t child_number,
                           const Network * net,
                           ScriptType key_type):PrivateKey(secret, true, net){
    init();
    type = key_type;
    memcpy(chainCode, chain_code, 32);
    depth = key_depth;
    childNumber = child_number;
    if(parent_fingerprint_arr != NULL){
        memcpy(parentFingerprint, parent_fingerprint_arr, 4);
    }else{
        memzero(parentFingerprint, 4);
    }
}
/*
HDPrivateKey &HDPrivateKey::operator=(const HDPrivateKey &other){
    if (this == &other){ return *this; } // self-assignment
    init();
    type = other.type;
    uint8_t secret[32];
    other.getSecret(secret);
    setSecret(secret);
    memcpy(chainCode, other.chainCode, 32);
    depth = other.depth;
    childNumber = other.childNumber;
    memcpy(parentFingerprint, other.parentFingerprint, 4);
    return *this;
};*/
HDPrivateKey::HDPrivateKey(const char * xprvArr){
    init();
    from_str(xprvArr, strlen(xprvArr));
}
HDPrivateKey::HDPrivateKey(const char * mnemonic, size_t mnemonicSize, const char * password, size_t passwordSize, const Network * net, void (*progress_callback)(float)){
    init();
    fromMnemonic(mnemonic, mnemonicSize, password, passwordSize, net, progress_callback);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
HDPrivateKey::HDPrivateKey(String mnemonic, String password, const Network * net, void (*progress_callback)(float)){
    init();
    fromMnemonic(mnemonic, password, net, progress_callback);
}
#endif
HDPrivateKey::~HDPrivateKey(void) {
    memzero(chainCode, 32);
    memzero(num, 32);
}
size_t HDPrivateKey::to_bytes(uint8_t * arr, size_t len) const{
    uint8_t hex[78] = { 0 };
    switch(type){
        case P2WPKH:
            memcpy(hex, network->zprv, 4);
            break;
        case P2SH_P2WPKH:
            memcpy(hex, network->yprv, 4);
            break;
        case P2WSH:
            memcpy(hex, network->Zprv, 4);
            break;
        case P2SH_P2WSH:
            memcpy(hex, network->Yprv, 4);
            break;
        default:
            memcpy(hex, network->xprv, 4);
    }
    hex[4] = depth;
    memcpy(hex+5, parentFingerprint, 4);
    for(uint8_t i=0; i<4; i++){
        hex[12-i] = ((childNumber >> (i*8)) & 0xFF);
    }
    memcpy(hex+13, chainCode, 32);
    memcpy(hex+46, num, 32);
    if(len > sizeof(hex)){
        len = sizeof(hex);
    }
    memcpy(arr, hex, len);
    return len;
}
size_t HDPrivateKey::to_stream(SerializeStream *s, size_t offset) const{
    uint8_t hex[78] = { 0 };
    size_t bytes_written = 0;
    to_bytes(hex, sizeof(hex));
    while(s->available() && bytes_written+offset < sizeof(hex)){
        s->write(hex[bytes_written+offset]);
        bytes_written++;
    }
    return bytes_written;
}
size_t HDPrivateKey::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    // reading the prefix
    while(s->available() > 0 && bytes_parsed+bytes_read < 4){
        prefix[bytes_parsed+bytes_read] = s->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == 4){
        bool found = false;
        for(int i=0; i<networks_len; i++){
            if(memcmp(prefix, networks[i]->xprv, 4)==0){
                type = UNKNOWN_TYPE;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->yprv, 4)==0){
                type = P2SH_P2WPKH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->zprv, 4)==0){
                type = P2WPKH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->Yprv, 4)==0){
                type = P2SH_P2WSH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->Zprv, 4)==0){
                type = P2WSH;
                found = true;
                network = networks[i];
                break;
            }
        }
        if(!found){
            status = PARSING_FAILED;
            return bytes_read;
        }
    }
    // depth
    if(s->available() > 0 && bytes_parsed+bytes_read < 5){
        depth = s->read();
        bytes_read++;
    }
    // fingerprint
    while(s->available() > 0 && bytes_parsed+bytes_read < 9){
        parentFingerprint[bytes_parsed+bytes_read-5] = s->read();
        bytes_read++;
    }
    // childnumber
    while(s->available() > 0 && bytes_parsed+bytes_read < 13){
        childNumber <<= 8;
        childNumber += s->read();
        bytes_read++;
    }
    // chaincode
    while(s->available() > 0 && bytes_parsed+bytes_read < 45){
        chainCode[bytes_parsed+bytes_read-13] = s->read();
        bytes_read++;
    }
    // 00 before the private key
    if(s->available() && bytes_parsed+bytes_read < 46){
        uint8_t c = s->read();
        bytes_read++;
        if(c != 0){
            status = PARSING_FAILED;
            return bytes_read;
        }
    }
    // num
    while(s->available() > 0 && bytes_parsed+bytes_read < 78){
        num[bytes_parsed+bytes_read-46] = s->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == 78){
        status = PARSING_DONE;
        uint8_t zero[32] = { 0 };
        if(memcmp(num, zero, 32)==0){ // should we add something else here?
            status = PARSING_FAILED;
        }
        bignum256 n;
        bn_read_be(num, &n);
        bn_mod(&n, &secp256k1.order);
        bn_write_be(&n, num);
        pubKey = *this * GeneratorPoint;
        pubKey.compressed = true;
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}
size_t HDPrivateKey::from_str(const char * xprvArr, size_t xprvLen){
    init();
    uint8_t arr[85] = { 0 };
    size_t l = fromBase58Check(xprvArr, xprvLen, arr, sizeof(arr));
    if(l == 0){
        return 0; // decoding error
    }
    ParseByteStream s(arr, sizeof(arr));
    HDPrivateKey::from_stream(&s);
    return xprvLen;
}
int HDPrivateKey::fromSeed(const uint8_t * seed, size_t seedSize, const Network * net){
    init();
    uint8_t raw[64] = { 0 };
    SHA512 sha;
    char key[] = "Bitcoin seed";
    sha.beginHMAC((uint8_t *)key, strlen(key));
    sha.write(seed, seedSize);
    sha.endHMAC(raw);
    // sha512Hmac((byte *)key, strlen(key), seed, 64, raw);
    memcpy(num, raw, 32);
    network = net;
    memcpy(chainCode, raw+32, 32);
    pubKey = *this * GeneratorPoint;
    pubKey.compressed = true;
    return 1;
}
// int HDPrivateKey::fromSeed(const uint8_t seed[64], const Network * net){
//     fromSeed(seed, 64);
// }
int HDPrivateKey::fromMnemonic(const char * mnemonic, size_t mnemonicSize, const char * password, size_t passwordSize, const Network * net, void (*progress_callback)(float)){
    init();
    uint8_t seed[64] = { 0 };
    uint8_t ind[4] = { 0, 0, 0, 1 };
    char salt[] = "mnemonic";
    uint8_t u[64] = { 0 };

    // first round
    SHA512 sha;
    sha.beginHMAC((uint8_t *)mnemonic, mnemonicSize);
    sha.write((uint8_t *)salt, strlen(salt));
    sha.write((uint8_t *)password, passwordSize);
    sha.write(ind, sizeof(ind));
    sha.endHMAC(u);
    memcpy(seed, u, 64);
    // other rounds
    for(int i=1; i<PBKDF2_ROUNDS; i++){
        if(progress_callback != NULL && (i & 0xFF) == 0xFF){
            progress_callback((float)i/(float)(PBKDF2_ROUNDS-1));
        }
        sha.beginHMAC((uint8_t *)mnemonic, mnemonicSize);
        sha.write(u, sizeof(u));
        sha.endHMAC(u);
        for(size_t j=0; j<sizeof(seed); j++){
            seed[j] = seed[j] ^ u[j];
        }
    }
    fromSeed(seed, sizeof(seed), net);
    return 1;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
int HDPrivateKey::fromMnemonic(String mnemonic, String password, const Network * net, void (*progress_callback)(float)){
    return fromMnemonic(mnemonic.c_str(), mnemonic.length(), password.c_str(), password.length(), net, progress_callback);
}
#endif
int HDPrivateKey::xprv(char * arr, size_t len) const{
    uint8_t hex[78] = { 0 };
    to_bytes(hex, sizeof(hex));
    return toBase58Check(hex, sizeof(hex), arr, len);
}
int HDPrivateKey::address(char * addr, size_t len) const{
    switch(type){
        case P2WPKH:
            return segwitAddress(addr, len);
        case P2SH_P2WPKH:
            return nestedSegwitAddress(addr, len);
        case P2PKH:
            return legacyAddress(addr, len);
        default:
            return segwitAddress(addr, len);
    }
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String HDPrivateKey::xprv() const{
    char arr[112] = { 0 };
    xprv(arr, sizeof(arr));
    return String(arr);
}
String HDPrivateKey::address() const{
    switch(type){
        case P2WPKH:
            return segwitAddress();
        case P2SH_P2WPKH:
            return nestedSegwitAddress();
        case P2PKH:
            return legacyAddress();
        default:
            return segwitAddress();
    }
}
#endif
int HDPrivateKey::xpub(char * arr, size_t len) const{
    uint8_t hex[111] = { 0 }; // TODO: real length, in xpub compressed = true
    switch(type){
        case P2WPKH:
            memcpy(hex, network->zpub, 4);
            break;
        case P2SH_P2WPKH:
            memcpy(hex, network->ypub, 4);
            break;
        case P2WSH:
            memcpy(hex, network->Zpub, 4);
            break;
        case P2SH_P2WSH:
            memcpy(hex, network->Ypub, 4);
            break;
        default:
            memcpy(hex, network->xpub, 4);
    }
    hex[4] = depth;
    memcpy(hex+5, parentFingerprint, 4);
    for(uint8_t i=0; i<4; i++){
        hex[12-i] = ((childNumber >> (i*8)) & 0xFF);
    }
    memcpy(hex+13, chainCode, 32);

    uint8_t sec[65] = { 0 };
    int secLen = publicKey().sec(sec, sizeof(sec));
    memcpy(hex+45, sec, secLen);
    return toBase58Check(hex, 45+secLen, arr, len);
}

void HDPrivateKey::fingerprint(uint8_t arr[4]) const{
    uint8_t secArr[65] = { 0 };
    int l = publicKey().sec(secArr, sizeof(secArr));
    uint8_t hash[20] = { 0 };
    hash160(secArr, l, hash);
    memcpy(arr, hash, 4);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String HDPrivateKey::fingerprint() const{
    uint8_t arr[4];
    fingerprint(arr);
    return toHex(arr, 4);
}
#endif

HDPublicKey HDPrivateKey::xpub() const{
    PublicKey p = publicKey();
    return HDPublicKey(p.point, chainCode, depth, parentFingerprint, childNumber, network, type);
}
HDPrivateKey HDPrivateKey::child(uint32_t index, bool hardened) const{
    if(index >= HARDENED_INDEX){
        hardened = true;
    }
    HDPrivateKey child;

    uint8_t sec[65] = { 0 };
    PublicKey p = publicKey();
    int l = p.sec(sec, sizeof(sec));
    uint8_t hash[20] = { 0 };
    hash160(sec, l, hash);
    memcpy(child.parentFingerprint, hash, 4);
    if(hardened && index < HARDENED_INDEX){
        index += HARDENED_INDEX;
    }
    child.childNumber = index;
    child.depth = depth+1;

    child.type = type;
    child.network = network;
    if(hardened){
        if(depth == 0){
            switch(index){
                case HARDENED_INDEX+44:
                    child.type = P2PKH;
                    break;
                case HARDENED_INDEX+49:
                    child.type = P2SH_P2WPKH;
                    break;
                case HARDENED_INDEX+84:
                    child.type = P2WPKH;
                    break;
                case HARDENED_INDEX+48:
                    child.type = MULTISIG;
                    break;
                case HARDENED_INDEX+45:
                    child.type = P2SH;
                    break;
            }
        }
        if(depth == 1){
            if(index == (HARDENED_INDEX+1)){
                child.network = &Testnet;
            }
            if(index == HARDENED_INDEX){
                child.network = &Mainnet;
            }
        }
        if(depth == 3 && type == MULTISIG){
            if(index == (HARDENED_INDEX+1)){
                child.type = P2SH_P2WSH;
            }
            if(index == (HARDENED_INDEX+2)){
                child.type = P2WSH;
            }
        }
    }

    uint8_t data[37];
    if(hardened){
        data[0] = 0;
        getSecret(data+1);
    }else{
        memcpy(data, sec, 33);
    }
    intToBigEndian(index, data+33, 4);

    uint8_t raw[64];
    SHA512 sha;
    sha.beginHMAC(chainCode, sizeof(chainCode));
    sha.write(data, 37);
    sha.endHMAC(raw);

    memcpy(child.chainCode, raw+32, 32);

    ECScalar r(raw, 32);
    r += *this;
    uint8_t secret[32];
    r.getSecret(secret);
    child.setSecret(secret);
    memzero(secret, 32);
    return child;
}

HDPrivateKey HDPrivateKey::hardenedChild(uint32_t index) const{
    return child(index, true);
}

HDPrivateKey HDPrivateKey::derive(uint32_t * index, size_t len) const{
    HDPrivateKey pk = *this;
    for(size_t i=0; i<len; i++){
        pk = pk.child(index[i]);
    }
    return pk;
}
HDPrivateKey HDPrivateKey::derive(const char * path) const{
    static const char VALID_CHARS[] = "0123456789/'h";
    size_t len = strlen(path);
    const char * cur = path;
    if(path[0] == 'm'){ // remove leading "m/"
        cur+=2;
        len-=2;
    }
    if(cur[len-1] == '/'){ // remove trailing "/"
        len--;
    }
    HDPrivateKey pk; // invalid private key to return if something failed
    size_t derivationLen = 1;
    // checking if all chars are valid and counting derivation length
    for(size_t i=0; i<len; i++){
        const char * pch = strchr(VALID_CHARS, cur[i]);
        if(pch == NULL){ // wrong character
            return pk;
        }
        if(cur[i] == '/'){
            derivationLen++;
        }
    }
    uint32_t * derivation = (uint32_t *)calloc(derivationLen, sizeof(uint32_t));
    if(derivation == NULL){ return pk; }
    size_t current = 0;
    for(size_t i=0; i<len; i++){
        if(cur[i] == '/'){ // next
            current++;
            continue;
        }
        const char * pch = strchr(VALID_CHARS, cur[i]);
        uint32_t val = pch-VALID_CHARS;
        if(derivation[current] >= HARDENED_INDEX){ // can't have anything after hardened
            free(derivation);
            return pk;
        }
        if(val < 10){
            derivation[current] = derivation[current]*10 + val;
        }else{ // h or ' -> hardened
            derivation[current] += HARDENED_INDEX;
        }
    }
    pk = derive(derivation, derivationLen);
    free(derivation);
    return pk;
}
// ---------------------------------------------------------------- HDPublicKey class

size_t HDPublicKey::to_bytes(uint8_t * arr, size_t len) const{
    uint8_t hex[78] = { 0 };
    switch(type){
        case P2WPKH:
            memcpy(hex, network->zpub, 4);
            break;
        case P2SH_P2WPKH:
            memcpy(hex, network->ypub, 4);
            break;
        case P2WSH:
            memcpy(hex, network->Zpub, 4);
            break;
        case P2SH_P2WSH:
            memcpy(hex, network->Ypub, 4);
            break;
        default:
            memcpy(hex, network->xpub, 4);
    }
    hex[4] = depth;
    memcpy(hex+5, parentFingerprint, 4);
    for(uint8_t i=0; i<4; i++){
        hex[12-i] = ((childNumber >> (i*8)) & 0xFF);
    }
    memcpy(hex+13, chainCode, 32);
    memcpy(hex+46, point, 32);
    hex[45] = 0x02 + (point[63] & 0x01);
    if(len > sizeof(hex)){
        len = sizeof(hex);
    }
    memcpy(arr, hex, len);
    return len;
}
size_t HDPublicKey::to_stream(SerializeStream *s, size_t offset) const{
    uint8_t hex[78] = { 0 };
    size_t bytes_written = 0;
    to_bytes(hex, sizeof(hex));
    while(s->available() && bytes_written+offset < sizeof(hex)){
        s->write(hex[bytes_written+offset]);
        bytes_written++;
    }
    return bytes_written;
}
size_t HDPublicKey::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    // reading the prefix
    while(s->available() > 0 && bytes_parsed+bytes_read < 4){
        prefix[bytes_parsed+bytes_read] = s->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == 4){
        bool found = false;
        for(int i=0; i<networks_len; i++){
            if(memcmp(prefix, networks[i]->xpub, 4)==0){
                type = UNKNOWN_TYPE;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->ypub, 4)==0){
                type = P2SH_P2WPKH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->zpub, 4)==0){
                type = P2WPKH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->Ypub, 4)==0){
                type = P2SH_P2WSH;
                found = true;
                network = networks[i];
                break;
            }else if(memcmp(prefix, networks[i]->Zpub, 4)==0){
                type = P2WSH;
                found = true;
                network = networks[i];
                break;
            }
        }
        if(!found){
            status = PARSING_FAILED;
            return bytes_read;
        }
    }
    // depth
    if(s->available() > 0 && bytes_parsed+bytes_read < 5){
        depth = s->read();
        bytes_read++;
    }
    // fingerprint
    while(s->available() > 0 && bytes_parsed+bytes_read < 9){
        parentFingerprint[bytes_parsed+bytes_read-5] = s->read();
        bytes_read++;
    }
    // childnumber
    while(s->available() > 0 && bytes_parsed+bytes_read < 13){
        childNumber <<= 8;
        childNumber += s->read();
        bytes_read++;
    }
    // chaincode
    while(s->available() > 0 && bytes_parsed+bytes_read < 45){
        chainCode[bytes_parsed+bytes_read-13] = s->read();
        bytes_read++;
    }
    // pubkey
    while(s->available() > 0 && bytes_parsed+bytes_read < 78){
        point[bytes_parsed+bytes_read-45] = s->read();
        bytes_read++;
    }
    // uncompressing the pubkey
    if(bytes_parsed+bytes_read == 78){
        status = PARSING_DONE;
        uint8_t arr[33];
        memcpy(arr, point, 33);
        uint8_t buf[65];
        ecdsa_uncompress_pubkey(&secp256k1, arr, buf);
        memcpy(point, buf+1, 64);
        if(!isValid()){
            status = PARSING_FAILED;
        }
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}
size_t HDPublicKey::from_str(const char * xpubArr, size_t xpubLen){
    uint8_t arr[85] = { 0 };
    size_t l = fromBase58Check(xpubArr, xpubLen, arr, sizeof(arr));
    if(l == 0){
        return 0; // decoding error
    }
    ParseByteStream s(arr, sizeof(arr));
    HDPublicKey::from_stream(&s);
    return xpubLen;
}

HDPublicKey::HDPublicKey():PublicKey(){
    memzero(prefix, 4);
    compressed = true;
    memzero(chainCode, 32);
    depth = 0;
    memzero(parentFingerprint, 4);
    childNumber = 0;
    network = &DEFAULT_NETWORK;
    type = UNKNOWN_TYPE;
}
HDPublicKey::HDPublicKey(const uint8_t p[64],
                           const uint8_t chain_code[32],
                           uint8_t key_depth,
                           const uint8_t parent_fingerprint_arr[4],
                           uint32_t child_number,
                           const Network * net,
                           ScriptType key_type):HDPublicKey(){
    memcpy(point, p, 64);
    compressed = true;
    type = key_type;
    network = net;
    memcpy(chainCode, chain_code, 32);
    depth = key_depth;
    childNumber = child_number;
    if(parent_fingerprint_arr != NULL){
        memcpy(parentFingerprint, parent_fingerprint_arr, 4);
    }else{
        memzero(parentFingerprint, 4);
    }
}
/*
HDPublicKey &HDPublicKey::operator=(const HDPublicKey &other){
    if (this == &other){ return *this; } // self-assignment
    type = other.type;
    memcpy(point, other.point, 64);
    memcpy(chainCode, other.chainCode, 32);
    compressed = true;
    depth = other.depth;
    childNumber = other.childNumber;
    memcpy(parentFingerprint, other.parentFingerprint, 4);
    return *this;
};*/
HDPublicKey::HDPublicKey(const char * xpubArr):HDPublicKey(){
    network = &DEFAULT_NETWORK;
    from_str(xpubArr, strlen(xpubArr));
}
HDPublicKey::~HDPublicKey(void) {
    memzero(point, 64);
    memzero(chainCode, 32);
}
int HDPublicKey::xpub(char * arr, size_t len) const{
    uint8_t hex[78] = { 0 };
    HDPublicKey::to_bytes(hex, sizeof(hex));
    return toBase58Check(hex, sizeof(hex), arr, len);
}
int HDPublicKey::address(char * addr, size_t len) const{
    switch(type){
        case P2WPKH:
            return PublicKey::segwitAddress(addr, len, network);
        case P2SH_P2WPKH:
            return PublicKey::nestedSegwitAddress(addr, len, network);
        case P2PKH:
            return PublicKey::legacyAddress(addr, len, network);
        default:
            return PublicKey::segwitAddress(addr, len, network);
    }
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String HDPublicKey::xpub() const{
    char arr[114] = { 0 };
    xpub(arr, sizeof(arr));
    return String(arr);
}
String HDPublicKey::address() const{
    switch(type){
        case P2WPKH:
            return PublicKey::segwitAddress(network);
        case P2SH_P2WPKH:
            return PublicKey::nestedSegwitAddress(network);
        case P2PKH:
            return PublicKey::legacyAddress(network);
        default:
            return PublicKey::segwitAddress(network);
    }
}
#endif

void HDPublicKey::fingerprint(uint8_t arr[4]) const{
    uint8_t secArr[65] = { 0 };
    int l = sec(secArr, sizeof(secArr));
    uint8_t hash[20] = { 0 };
    hash160(secArr, l, hash);
    memcpy(arr, hash, 4);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String HDPublicKey::fingerprint() const{
    uint8_t arr[4];
    fingerprint(arr);
    return toHex(arr, 4);
}
#endif

HDPublicKey HDPublicKey::child(uint32_t index) const{
    HDPublicKey child;

    uint8_t secArr[65] = { 0 };
    int l = sec(secArr, sizeof(secArr));
    uint8_t hash[20] = { 0 };
    hash160(secArr, l, hash);
    memcpy(child.parentFingerprint, hash, 4);
    child.childNumber = index;
    child.depth = depth+1;

    child.type = type;
    child.network = network;

    uint8_t data[37];
    memcpy(data, secArr, 33);
    intToBigEndian(index, data+33, 4);

    uint8_t raw[64];
    SHA512 sha;
    sha.beginHMAC(chainCode, sizeof(chainCode));
    sha.write(data, 37);
    sha.endHMAC(raw);

    memcpy(child.chainCode, raw+32, 32);

    ECScalar r(raw, 32);
    ECPoint p = r*GeneratorPoint;
    p += *this;
    memcpy(child.point, p.point, 64);
    child.compressed = true;
    return child;
}
HDPublicKey HDPublicKey::derive(uint32_t * index, size_t len) const{
    HDPublicKey pk = *this;
    for(size_t i=0; i<len; i++){
        pk = pk.child(index[i]);
    }
    return pk;
}
HDPublicKey HDPublicKey::derive(const char * path) const{
    static const char VALID_CHARS[] = "0123456789/";
    size_t len = strlen(path);
    const char * cur = path;
    if(path[0] == 'm'){ // remove leading "m/"
        cur+=2;
        len-=2;
    }
    if(cur[len-1] == '/'){ // remove trailing "/"
        len--;
    }
    HDPublicKey pk; // dummy to return if something failed
    size_t derivationLen = 1;
    // checking if all chars are valid and counting derivation length
    for(size_t i=0; i<len; i++){
        const char * pch = strchr(VALID_CHARS, cur[i]);
        if(pch == NULL){ // wrong character
            return pk;
        }
        if(cur[i] == '/'){
            derivationLen++;
        }
    }
    uint32_t * derivation = (uint32_t *)calloc(derivationLen, sizeof(uint32_t));
    if(derivation == NULL){ return pk; }
    size_t current = 0;
    for(size_t i=0; i<len; i++){
        if(cur[i] == '/'){ // next
            if(derivation[current] >= HARDENED_INDEX){ // can't be hardened
                free(derivation);
                return pk;
            }
            current++;
            continue;
        }
        const char * pch = strchr(VALID_CHARS, cur[i]);
        uint32_t val = pch-VALID_CHARS;
        derivation[current] = derivation[current]*10 + val;
    }
    pk = derive(derivation, derivationLen);
    free(derivation);
    return pk;
}

