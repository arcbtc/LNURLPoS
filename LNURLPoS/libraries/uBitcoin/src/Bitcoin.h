/** @file Bitcoin.h
 */

#ifndef __BITCOIN_H__
#define __BITCOIN_H__

#include "uBitcoin_conf.h"
#include "BaseClasses.h"
#include "BitcoinCurve.h"
#include "Conversion.h"
#include "utility/trezor/rand.h"
#include <stdint.h>
#include <string.h>

/* TODO:
   - autodetect mnemonic w/o passwd or xprv
   - HD.derive()
   - accept strings instead of char arrays for txout (address) and other things
   - fail if script or witness is too large
   - fix is_canonical function
   - refactor fromWIF to return bytes read
   - fix all warnings
   - psbt
   - docs
   - publish on arduino libs and mbed
   - operators +, += in script - concatenation
   - signature & everything from char array might be not a bright idea
   - tests (egde cases!)
   - sidechannel for pubkey calculation - use rng
 */

/** \brief Prefixes for particular network (Mainnet / Testnet ).<br>
 *  HD key prefixes are described here:<br>
 *  https://github.com/satoshilabs/slips/blob/master/slip-0132.md<br>
 *  useful tool: in https://iancoleman.io/bip39/
 */
typedef struct {
    /** \brief Pay-To-Pubkey-Hash addresses */
    uint8_t p2pkh;   
    /** \brief Pay-To-Script-Hash addresses */
    uint8_t p2sh;    
    /** \brief Prefix for segwit addreses ...for regtest it is larger */
    char bech32[5];  
    /** \brief Wallet Import Format, used in PrivateKey */
    uint8_t wif;     
    /** \brief HD private key for legacy addresses (P2PKH) */
    uint8_t xprv[4]; 
    /** \brief HD private key for nested Segwit (P2SH-P2WPKH) */
    uint8_t yprv[4]; 
    /** \brief HD private key for native Segwit (P2WPKH) */
    uint8_t zprv[4]; 
    /** \brief HD private key for nested Segwit Multisig (P2SH-P2WSH) */
    uint8_t Yprv[4]; 
    /** \brief HD private key for native Segwit Multisig (P2WSH) */
    uint8_t Zprv[4]; 
    /** \brief HD public key for legacy addresses (P2PKH) */
    uint8_t xpub[4]; 
    /** \brief HD public key for nested Segwit (P2SH-P2WPKH) */
    uint8_t ypub[4]; 
    /** \brief HD public key for native Segwit (P2WPKH) */
    uint8_t zpub[4]; 
    /** \brief HD public key for nested Segwit Multisig (P2SH-P2WSH) */
    uint8_t Ypub[4]; 
    /** \brief HD public key for native Segwit Multisig (P2WSH) */
    uint8_t Zpub[4]; 
    /** \brief bip32 coin index */
    uint32_t bip32;
} Network;

extern const Network Mainnet;
extern const Network Testnet;
extern const Network Regtest;
extern const Network Signet;

extern const Network * networks[];
extern const uint8_t networks_len;

extern int ubtc_errno;

// number of rounds for mnemonic to seed conversion
#define PBKDF2_ROUNDS 2048
#define HARDENED_INDEX 0x80000000

/** \brief Common script types */
enum ScriptType{
    UNKNOWN_TYPE,
    /**  \brief a script directly in ScriptPubkey and not one of below */
    DIRECT_SCRIPT,
    /**  \brief default script for signing */
    P2PKH,
    P2SH,
    P2WPKH,
    P2WSH,
    P2SH_P2WPKH,
    P2SH_P2WSH,
    MULTISIG
};

/** \brief SigHash types */
enum SigHashType{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3
};

/* forward declarations */
class Signature;
class PublicKey;
class PrivateKey;
class HDPublicKey;
class HDPrivateKey;
class Script;
class TxIn;

const char * generateMnemonic(uint8_t numWords);
const char * generateMnemonic(uint8_t numWords, const uint8_t * entropy_data, size_t dataLen);
const char * generateMnemonic(const uint8_t * entropy_data, size_t dataLen);
#if USE_ARDUINO_STRING
const char * generateMnemonic(uint8_t numWords, const String entropy_string);
const char * generateMnemonic(const String entropy_string);
bool checkMnemonic(const String mnemonic);
#elif USE_STD_STRING
const char * generateMnemonic(uint8_t numWords, const std::string entropy_string);
const char * generateMnemonic(const std::string entropy_string);
bool checkMnemonic(const std::string mnemonic);
#else
const char * generateMnemonic(uint8_t numWords, const char * entropy_string);
const char * generateMnemonic(const char * entropy_string);
bool checkMnemonic(const char * mnemonic);
#endif

const char * mnemonicFromEntropy(const uint8_t * entropy_data, size_t dataLen);
size_t mnemonicToEntropy(const char * mnemonic, size_t mnemonic_len, uint8_t * output, size_t outputLen);
#if USE_ARDUINO_STRING
size_t mnemonicToEntropy(String mnemonic, uint8_t * output, size_t outputLen);
#elif USE_STD_STRING
size_t mnemonicToEntropy(std::string mnemonic, uint8_t * output, size_t outputLen);
#else
size_t mnemonicToEntropy(char * mnemonic, uint8_t * output, size_t outputLen);
#endif

/**
 *  PublicKey class.
 *
 *  Derived from ECPoint class, therefore you can add or substract them, multiply by ECScalar or PrivateKey.
 *
 *  `compressed` flag determines what public key sec format to use by default:
 *  - `compressed = false` will use 65-byte representation (`04<x><y>`)
 *  - `compressed = true` will use 33-byte representation (`03<x>` if y is odd, `02<x>` if y is even)
 */
class PublicKey : public ECPoint{
public:
    PublicKey():ECPoint(){};
    PublicKey(const uint8_t pubkeyArr[64], bool use_compressed) : ECPoint(pubkeyArr, use_compressed){};
    PublicKey(const uint8_t * secArr) : ECPoint(secArr){};
    explicit PublicKey(const char * secHex) : ECPoint(secHex){};
    // do I need this?
    PublicKey(ECPoint p):PublicKey(p.point, p.compressed){};
    /**
     *  \brief Fills `addr` with legacy Pay-To-Pubkey-Hash address (P2PKH, `1...` for mainnet)
     */
    int legacyAddress(char * addr, size_t len, const Network * network = &DEFAULT_NETWORK) const;
    /**
     *  \brief Fills `addr` with native segwit address (P2WPKH, `bc1...` for mainnet)
     */
    int segwitAddress(char * addr, size_t len, const Network * network = &DEFAULT_NETWORK) const;
    /**
     *  \brief Fills `addr` with nested segwit address (P2SH-P2WPKH, `3...` for mainnet)
     */
    int nestedSegwitAddress(char * addr, size_t len, const Network * network = &DEFAULT_NETWORK) const;
    /**
     *  \brief Alias for `legacyAddress`
     */
    int address(char * address, size_t len, const Network * network = &DEFAULT_NETWORK) const{ return legacyAddress(address, len, network); };
#if USE_ARDUINO_STRING
    String legacyAddress(const Network * network = &DEFAULT_NETWORK) const;
    String segwitAddress(const Network * network = &DEFAULT_NETWORK) const;
    String nestedSegwitAddress(const Network * network = &DEFAULT_NETWORK) const;
    String address(const Network * network = &DEFAULT_NETWORK) const{ return legacyAddress(network); };
#endif
#if USE_STD_STRING
    std::string legacyAddress(const Network * network = &DEFAULT_NETWORK) const;
    std::string segwitAddress(const Network * network = &DEFAULT_NETWORK) const;
    std::string nestedSegwitAddress(const Network * network = &DEFAULT_NETWORK) const;
    std::string address(const Network * network = &DEFAULT_NETWORK) const{ return legacyAddress(network); };
#endif
    /**
     *  \brief verifies the ECDSA signature of the hash of the message
     */
    bool verify(const Signature sig, const uint8_t hash[32]) const;
    /**
     *  \brief Returns a Script with the type: `P2PKH`, `P2WPKH` or `P2SH_P2WPKH`
     */
    Script script(ScriptType type = P2PKH) const;
};

/**
 *  PrivateKey class.
 *  Corresponding public key (point on curve) will be calculated in the constructor.
 *      as point calculation is pretty slow, class initialization can take some time.
 */
class PrivateKey : public ECScalar{
protected:
    /** \brief corresponding point on curve ( secret * G ) */
    PublicKey pubKey;
    virtual size_t to_str(char * buf, size_t len) const{ return wif( buf, len); };
    virtual size_t from_str(const char * buf, size_t len){ return fromWIF(buf, len); };
    virtual size_t from_stream(ParseStream *s);
public:
    PrivateKey();
    PrivateKey(const uint8_t secret_arr[32], bool use_compressed = true, const Network * net = &DEFAULT_NETWORK);
#if USE_ARDUINO_STRING
    PrivateKey(const String wifString);
#elif USE_STD_STRING
    PrivateKey(const std::string wifString);
#else
    PrivateKey(const char * wifArr);
#endif
    ~PrivateKey();
    /** \brief Length of the key in WIF format (52). In reality not always 52... */
    virtual size_t stringLength() const{ return 52; };
    virtual size_t length() const{ return 32; };
    void setSecret(const uint8_t secret_arr[32]){ memcpy(num, secret_arr, 32); pubKey = *this * GeneratorPoint; };

    /** \brief Pointer to the network to use. Mainnet or Testnet */
    const Network * network;

    /** \brief Writes the private key in Wallet Import Format */
    int wif(char * wifArr, size_t len) const;
#if USE_ARDUINO_STRING
    String wif() const;
#endif
#if USE_STD_STRING
    std::string wif() const;
#endif
    /** \brief Loads the private key from a string in Wallet Import Format */
    int fromWIF(const char * wifArr, size_t wifSize);
    int fromWIF(const char * wifArr);
    /** \brief Returns the corresponding PublicKey = secret * GeneratorPoint */
    PublicKey publicKey() const;
    /** \brief Signs the hash and returns the Signature */
    Signature sign(const uint8_t hash[32]) const; // pass 32-byte hash of the message here

    /** \brief Alias for .publicKey().address(network) */
    int address(char * address, size_t len) const;
    /** \brief Alias for .publicKey().legacyAddress(network) */
    int legacyAddress(char * address, size_t len) const;
    /** \brief Alias for .publicKey().segwitAddress(network) */
    int segwitAddress(char * address, size_t len) const;
    /** \brief Alias for .publicKey().nestedSegwitAddress(network) */
    int nestedSegwitAddress(char * address, size_t len) const;
#if USE_ARDUINO_STRING
    String address() const;
    String legacyAddress() const;
    String segwitAddress() const;
    String nestedSegwitAddress() const;
#endif
#if USE_STD_STRING
    std::string address() const;
    std::string legacyAddress() const;
    std::string segwitAddress() const;
    std::string nestedSegwitAddress() const;
#endif
//    PrivateKey &operator=(const PrivateKey &other);                   // assignment
};

/**
 *  \brief HD Private Key class. Derived from PrivateKey class.
 *         Works according to [bip32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki),
 *         [bip39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) and 
 *         [slip32](https://github.com/satoshilabs/slips/blob/master/slip-0032.md).
 *  You can generate the key from mnemonic or seed, derive children and hardened children.
 *  xprv and xpub methods return strings according to slip32, xprv/xpub for bip44, yprv/ypub for bip49 and zprv/zpub for bip84
 */
class HDPrivateKey : public PrivateKey{
protected:
    void init();
    size_t to_bytes(uint8_t * arr, size_t len) const;
    virtual size_t to_str(char * buf, size_t len) const{ return xprv( buf, len); };
    virtual size_t from_str(const char * buf, size_t len);
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint8_t prefix[4]; // used for parsing only
public:
    HDPrivateKey();
    HDPrivateKey(const uint8_t secret[32], const uint8_t chain_code[32],
                 uint8_t key_depth = 0,
                 const uint8_t parent_fingerprint_arr[4] = NULL,
                 uint32_t childnumber = 0,
                 const Network * network = &DEFAULT_NETWORK,
                 ScriptType key_type = UNKNOWN_TYPE);
    HDPrivateKey(const char xprvArr[]);
    HDPrivateKey(const char * mnemonic, size_t mnemonicSize, const char * password, size_t passwordSize, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
#if USE_STD_STRING
    HDPrivateKey(std::string mnemonic, std::string password, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
#endif
#if USE_ARDUINO_STRING
    HDPrivateKey(String mnemonic, String password, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
#endif
/*    HDPrivateKey(const HDPrivateKey &other):HDPrivateKey(  // copy
        other.num, other.chainCode, other.depth,
        other.parentFingerprint, other.childNumber, other.network, other.type){};
*/
    ~HDPrivateKey();
    virtual size_t length() const{ return 78; };
    /** \brief Length of the key in base58 encoding (111). */
    virtual size_t stringLength() const{ return 111; };

    uint8_t chainCode[32];
    uint8_t depth;
    uint8_t parentFingerprint[4];
    uint32_t childNumber;
    ScriptType type;

    int fromSeed(const uint8_t * seed, size_t seedSize, const Network * network = &DEFAULT_NETWORK);
    // int fromSeed(const uint8_t seed[64], const Network * network = &DEFAULT_NETWORK);
    int fromMnemonic(const char * mnemonic, size_t mnemonicSize, const char * password, size_t passwordSize, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
    int fromMnemonic(const char * mnemonic, const char * password, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL){
        return fromMnemonic(mnemonic, strlen(mnemonic), password, strlen(password), network, progress_callback);
    }
#if USE_STD_STRING
    int fromMnemonic(std::string mnemonic, std::string password, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
#endif
#if USE_ARDUINO_STRING
    int fromMnemonic(String mnemonic, String password, const Network * network = &DEFAULT_NETWORK, void (*progress_callback)(float) = NULL);
#endif
    int xprv(char * arr, size_t len) const;
    int xpub(char * arr, size_t len) const;
    HDPublicKey xpub() const;
    int address(char * arr, size_t len) const;
#if USE_ARDUINO_STRING
    String xprv() const;
    String address() const;
#endif
#if USE_STD_STRING
    std::string xprv() const;
    std::string address() const;
#endif

    /** \brief populates array with the fingerprint of the key */
    void fingerprint(uint8_t arr[4]) const;
#if USE_STD_STRING
    std::string fingerprint() const;
#endif
#if USE_ARDUINO_STRING
    String fingerprint() const;
#endif

    HDPrivateKey child(uint32_t index, bool hardened = false) const;
    HDPrivateKey hardenedChild(uint32_t index) const;
    /** \brief derives a child according to derivation path. Use 0x80000000 + index for hardened index. */
    HDPrivateKey derive(uint32_t * index, size_t len) const;
    /** \brief derives a child according to derivation path. For example "m/84h/1h/0h/1/23/" for the 23rd change address for testnet with P2WPKH type (bip84). */
    HDPrivateKey derive(const char * path) const;
#if USE_ARDUINO_STRING
    HDPrivateKey derive(String path) const{ return derive(path.c_str()); };
#endif
    // just to make sure it is compressed
    PublicKey publicKey() const{ PublicKey p = pubKey; p.compressed = true; return p; };
//    HDPrivateKey &operator=(const HDPrivateKey &other);                   // assignment
};

/**
 *  \brief HD Public Key class. Derived from PublicKey class.
 *         Works according to [bip32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki),
 *         [bip39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) and 
 *         [slip32](https://github.com/satoshilabs/slips/blob/master/slip-0032.md).
 *  You can derive children
 *  xpub method return strings according to slip32, xpub for bip44, ypub for bip49 and zpub for bip84
 */
class HDPublicKey : public PublicKey{
    size_t to_bytes(uint8_t * arr, size_t len) const;
    virtual size_t to_str(char * buf, size_t len) const{ return xpub( buf, len); };
    virtual size_t from_str(const char * buf, size_t len);
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint8_t prefix[4]; // used for parsing only
public:
    HDPublicKey();
    HDPublicKey(const uint8_t point[64], const uint8_t chain_code[32],
                 uint8_t key_depth = 0,
                 const uint8_t parent_fingerprint_arr[4] = NULL,
                 uint32_t childnumber = 0,
                 const Network * net = &DEFAULT_NETWORK,
                 ScriptType key_type = UNKNOWN_TYPE);
    HDPublicKey(const char * xpubArr);
/*    HDPublicKey(const HDPublicKey &other):HDPublicKey(  // copy
        other.point, other.chainCode, other.depth,
        other.parentFingerprint, other.childNumber, other.network, other.type){};
*/
#if USE_ARDUINO_STRING
    HDPublicKey(String pub){ from_str(pub.c_str(), pub.length()); };
#endif
    ~HDPublicKey();
    /** \brief Length of the key (78). */
    virtual size_t length() const{ return 78; };
    /** \brief Length of the key in base58 encoding (111). */
    virtual size_t stringLength() const{ return 111; };

    uint8_t chainCode[32];
    uint8_t depth;
    uint8_t parentFingerprint[4];
    uint32_t childNumber;
    ScriptType type;
    const Network * network;

    int xpub(char * arr, size_t len) const;
    int address(char * arr, size_t len) const;
#if USE_ARDUINO_STRING
    String xpub() const;
    String address() const;
#endif
#if USE_STD_STRING
    std::string xpub() const;
    std::string address() const;
#endif

    /** \brief populates array with the fingerprint of the key */
    void fingerprint(uint8_t arr[4]) const;
#if USE_STD_STRING
    std::string fingerprint() const;
#endif
#if USE_ARDUINO_STRING
    String fingerprint() const;
#endif

    /** \brief derive a child. 
     *         You can derive only normal children (not hardened) from the public key. 
     */
    HDPublicKey child(uint32_t index) const;
    /** \brief derives a child according to derivation path. */
    HDPublicKey derive(uint32_t * index, size_t len) const;
    /** \brief derives a child according to derivation path. For example "m/1/23/" for the 23rd change address. */
    HDPublicKey derive(const char * path) const;
#if USE_ARDUINO_STRING
    HDPublicKey derive(String path) const{ return derive(path.c_str()); };
#endif
//    HDPublicKey &operator=(const HDPublicKey &other);                   // assignment
};

/**
 *  \brief Signature class.
 *         Reference: https://github.com/bitcoin/bips/blob/master/bip-0066.mediawiki
 */
class Signature : public Streamable{
protected:
    uint8_t r[32];
    uint8_t s[32];
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    size_t rlen() const;
    size_t slen() const;
    uint8_t tot[3]; // temporary thingy for parsing
public:
    Signature();
    Signature(const uint8_t r_arr[32], const uint8_t s_arr[32]);
    Signature(const uint8_t * der, size_t derLen);
    Signature(const uint8_t * der);
    explicit Signature(const char * der);
    virtual size_t length() const;

    uint8_t index; // used to derive pubkey from signature

    /** \brief encodes signature in der format and writes it to array */
    size_t der(uint8_t * arr, size_t len) const;
    /** \brief parses signature in der format */
    size_t fromDer(const uint8_t * arr, size_t len);
    /** \brief populates array with <r[32]><s[32]><index> */
    void bin(uint8_t * arr, size_t len) const;
    /** \brief parses array as <r[32]><s[32]><index> */
    void fromBin(const uint8_t * arr, size_t len);

    bool isValid() const{ uint8_t arr[32] = { 0 }; return !((memcmp(r, arr, 32) == 0) && (memcmp(s, arr, 32)==0)); };
    explicit operator bool() const{ return isValid(); };

    bool operator==(const Signature& other) const{ return (memcmp(r, other.r, 32) == 0) && (memcmp(s, other.s, 32) == 0); };
    bool operator!=(const Signature& other) const{ return !operator==(other); };
};

/**
 *  \brief Script class. Parsing requires the length of the script in the beginning.
 */
class Script : public Streamable{
protected:
    uint8_t * scriptArray;
    size_t scriptLen;
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint8_t lenLen; // for parsing only, length of the varint
    void fromAddress(const char * address);
    void init();
public:
    void clear();
    Script();
    Script(const uint8_t * buffer, size_t len);
    /** \brief creates a script from address */
    Script(const char * address){ fromAddress(address); };
#if USE_ARDUINO_STRING
    /** \brief creates a script from address */
    Script(const String address){ fromAddress(address.c_str()); };
#endif
#if USE_STD_STRING
    /** \brief creates a script from address */
    Script(const std::string address){ fromAddress(address.c_str()); };
#endif
    /** \brief creates one of standart scripts (P2PKH, P2WPKH) */
    Script(const PublicKey pubkey, ScriptType type = P2PKH);
    /** \brief creates one of standart scripts (P2SH, P2WSH) */
    Script(const Script &other, ScriptType type);
    Script(const Script &other); // copy
    ~Script(){ if(scriptArray){ free(scriptArray); } };

    /** \brief tries to determine the script type */
    ScriptType type() const;
    /** \brief returns address corresponding to the script */
    size_t address(char * buffer, size_t len, const Network * network = &DEFAULT_NETWORK) const;
#if USE_ARDUINO_STRING
    String address(const Network * network = &DEFAULT_NETWORK) const;
#endif
#if USE_STD_STRING
    std::string address(const Network * network = &DEFAULT_NETWORK) const;
#endif

    /** \brief length of the script with varint */
    virtual size_t length() const;    
    /** \brief pushes a single byte (op_code) to the end */
    size_t push(uint8_t code);
    /** \brief pushes bytes from data object to the end */
    size_t push(const uint8_t * data, size_t len);
    /** \brief adds <len><sec> to the script */
    size_t push(const PublicKey pubkey);
    /** \brief adds <len><der><sigType> to the script */
    size_t push(const Signature sig, SigHashType sigType = SIGHASH_ALL);
    /** \brief adds <len><script> to the script (used for P2SH) */
    size_t push(const Script sc);

    /** \brief returns scriptPubkey for this scripts (P2SH or P2WSH) */
    Script scriptPubkey(ScriptType type = P2SH) const;

    Script &operator=(const Script &other);                   // assignment

    // Bool conversion. Allows to use if(script) construction. Returns false if script is empty, true otherwise
    explicit operator bool() const{ return (scriptLen > 0); };
    bool operator==(const Script& other) const{ return (scriptLen == other.scriptLen) && (memcmp(scriptArray, other.scriptArray, scriptLen) == 0); };
    bool operator!=(const Script& other) const{ return !operator==(other); };
};

/**
 *  \brief Witness class. Has a form of `<num><e0><e1><e2>...` 
 *         where `<e>` can be a public key, signature or arbitrary data (i.e. hash)
 */
class Witness : public Streamable{
    uint8_t * witnessArray;
    size_t witnessLen;
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint32_t numElements;
    // used for parsing only:
    uint32_t cur_element; // index of current element
    size_t cur_element_len; // length of current element
    size_t cur_bytes_parsed; // number of bytes read from current element
    uint8_t curLen; // for parsing only, length of the varint
    uint8_t lenLen; // for parsing, length of the varint
    virtual void reset(){ status = PARSING_DONE; bytes_parsed = 0; cur_element_len=0; cur_bytes_parsed=0; cur_element=0; lenLen=0; };
    void init();
public:
    void clear();
    virtual size_t length() const;
    Witness();
    Witness(const uint8_t * buffer, size_t len);
    Witness(const Signature sig, const PublicKey pub);
    Witness(const Witness &other); // copy
    ~Witness(){ if(witnessArray){ free(witnessArray); } };
    /** \brief returns number of elements in the witness */
    uint8_t count() const{ return numElements; };
    /** \brief adds `<len><data>` to the witness */
    size_t push(const uint8_t * data, size_t len);
    /** \brief adds `<len><sec>` to the witness */
    size_t push(const PublicKey pubkey);
    /** \brief adds `<len><der><sigType>` to the witness */
    size_t push(const Signature sig, SigHashType sigType = SIGHASH_ALL);
    /** \brief adds `<len><script>` to the witness */
    size_t push(const Script sc);

    Witness &operator=(Witness const &other); // assignment
    explicit operator bool() const{ return (numElements > 0); };
    bool operator==(const Witness& other) const{ return (witnessLen == other.witnessLen) && (memcmp(witnessArray, other.witnessArray, witnessLen) == 0) && (numElements == other.numElements); };
    bool operator!=(const Witness& other) const{ return !operator==(other); };
};

/**
 *  \brief Transaction Input class. Serializes as `<prev_hash><prev_index><scriptSig><sequence>`<br>
 *         Stores information about previous transaction hash, prev output number,
 *         scriptSig, sequence and witness data if it is segwit.
 */
class TxIn : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    void init();
public:
    TxIn(void);
    TxIn(const uint8_t prev_id[32], uint32_t prev_index, const Script script, uint32_t sequence_number = 0xffffffff);
    TxIn(const uint8_t prev_id[32], uint32_t prev_index, uint32_t sequence_number = 0xffffffff);
    explicit TxIn(const char * prev_id, uint32_t prev_index, const Script script, uint32_t sequence_number = 0xffffffff);
    explicit TxIn(const char * prev_id, uint32_t prev_index, uint32_t sequence_number = 0xffffffff);
    virtual size_t length() const;
    uint8_t hash[32];
    uint32_t outputIndex;
    Script scriptSig;
    uint32_t sequence;
    Witness witness;
    /** \brief checks if the input is segwit or not */
    bool isSegwit() const{ return (witness.count() > 0); };

    bool isValid() const{ return status==PARSING_DONE; };
    explicit operator bool() const{ return isValid(); };
};

/**
 *  \brief Transaction Output class.<br>
 *         Stores information the amount and ScriptPubkey,
 */
class TxOut : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    void init(){ status = PARSING_DONE; bytes_parsed=0; amount = 0; };
public:
    TxOut(){ amount = 0; };
    TxOut(uint64_t send_amount, const Script outputScript){ amount = send_amount; scriptPubkey = outputScript; };
    TxOut(const Script outputScript, uint64_t send_amount){ amount = send_amount; scriptPubkey = outputScript; };
    TxOut(uint64_t send_amount, const char * address){ amount = send_amount; scriptPubkey = Script(address); }; 
    TxOut(const char * address, uint64_t send_amount){ amount = send_amount; scriptPubkey = Script(address); };
    virtual size_t length() const{ return 8+scriptPubkey.length(); };

    /** \brief this script defines the rules for the spending input */
    Script scriptPubkey;
    /** \brief the output amount in satoshi */
    uint64_t amount;
    /** \brief returns the output amount in BTC */
    float btcAmount(){ return (float)amount/1e8; };
    /** \brief returns the address corresponding to the output script */
    size_t address(char * addr, size_t len, const Network * network = &DEFAULT_NETWORK) const{ return scriptPubkey.address(addr, len, network); };
#if USE_ARDUINO_STRING
    String address(const Network * network = &DEFAULT_NETWORK) const{ return scriptPubkey.address(network); };
#endif
#if USE_STD_STRING
    std::string address(const Network * network = &DEFAULT_NETWORK) const{ return scriptPubkey.address(network); };
#endif

    bool isValid() const{ return status==PARSING_DONE; };
    explicit operator bool() const{ return isValid(); };
};
/**
 *  \brief Transaction class.<br>
 *         Can be segwit or not. For legacy tx serializes as `<ver><inputsNumber><inputs><outputsNumber><outputs><locktime>`<br>
 *         For segwit tx serializes as `<ver><00><01><inputsNumber><inputs><outputsNumber><outputs><witnesses><locktime>`
 */
class Tx : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint8_t segwit_flag;
    void clear();
    void init();
public:
    Tx();
    Tx(Tx const &other);
    ~Tx();
    virtual size_t length() const;
    uint32_t version;
    size_t inputsNumber;
    TxIn * txIns;
    size_t outputsNumber;
    TxOut * txOuts;
    uint32_t locktime;

    /** \brief checks wether transaction is segwit or not.<br> 
      *        returns `true` if at least one of the inputs has non-empty witness  
      */
    bool isSegwit() const;

    /** \brief populates hash with transaction hash */
    int hash(uint8_t h[32]) const;
    /** \brief populates hash with transaction hash if serialized as segwit */
    int whash(uint8_t h[32]) const;
    /** \brief populates array with id of the transaction (reverse of the hash) */
    int txid(uint8_t id_arr[32]) const;
    /** \brief populates array with witness id of the transaction */
    int wtxid(uint8_t id_arr[32]) const;
#if USE_ARDUINO_STRING
    String txid() const;
    String wtxid() const;
#endif
#if USE_STD_STRING
    std::string txid() const;
    std::string wtxid() const;
#endif

    /** \brief adds another input to the transaction */
    uint8_t addInput(const TxIn txIn);
    /** \brief adds another output to the transaction */
    uint8_t addOutput(const TxOut txOut);

    /** \brief calculates a hash to sign for certain input */
    int sigHash(uint8_t h[32], uint8_t inputIndex, const Script scriptPubkey, SigHashType sighash = SIGHASH_ALL) const;

    int hashPrevouts(uint8_t h[32]) const;
    int hashSequence(uint8_t h[32]) const;
    int hashOutputs(uint8_t h[32]) const;
    int sigHashSegwit(uint8_t h[32], uint8_t inputIndex, const Script scriptPubKey, uint64_t amount, SigHashType sighash = SIGHASH_ALL) const;

#if 0
    /** \brief sorts inputs and outputs in alphabetical order */
    void sort();
#endif

    /** \brief signs legacy input with certain script and returns a signature.
     *         Don't forget to construct txIns[i].scriptSig correctly if you are using P2SH.
     *         For P2WPKH, P2WSH and P2SH-P2WPKH use signSegwitInput method.
     */
    Signature signInput(uint8_t inputIndex, const PrivateKey pk, const Script redeemScript, SigHashType sighash = SIGHASH_ALL);
    /** \brief signs legacy input and returns a signature */
    Signature signInput(uint8_t inputIndex, const PrivateKey pk){
        return signInput(inputIndex, pk, Script(pk.publicKey(), P2PKH));
    };

    /** \brief signs segwit input with certain script and returns a signature.
     *         Don't forget to construct txIns[i].witness correctly if you are using P2WSH or P2SH-P2WSH.
     *         For P2PKH and P2SH use signInput method.
     */
    Signature signSegwitInput(uint8_t inputIndex, const PrivateKey pk, const Script redeemScript, uint64_t amount, ScriptType type = P2WSH, SigHashType sighash = SIGHASH_ALL);
    /** \brief signs segwit input and returns a signature. Uses native segwit (P2WPKH) by default, 
     *         you can also specify the type to be P2SH-P2WPKH to sign nested segwit transaction.
     */
    Signature signSegwitInput(uint8_t inputIndex, const PrivateKey pk, uint64_t amount, ScriptType type = P2WPKH){
        return signSegwitInput(inputIndex, pk, Script(pk.publicKey(), P2WPKH), amount, type); // FIXME: are you sure?
    };

    Tx &operator=(Tx const &other);

    bool isValid() const{ return status==PARSING_DONE; };
    explicit operator bool() const{ return isValid(); };
};

#endif // __BITCOIN_H__
