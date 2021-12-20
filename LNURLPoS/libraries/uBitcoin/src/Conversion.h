#ifndef __CONVERSION_H__
#define __CONVERSION_H__

#include "uBitcoin_conf.h"

#include <string.h>
#include <stdint.h>
#include "utility/segwit_addr.h"


#define BASE64_STANDARD  0
#define BASE64_NOPADDING 1
#define BASE64_URLSAFE   2

// TODO: get rid of these blahLength functions, they are redundant
//       just stop when array is full and return errorcode

size_t toBase58Length(const uint8_t * array, size_t arraySize);
size_t toBase58(const uint8_t * array, size_t arraySize, char * output, size_t outputSize);
#if USE_ARDUINO_STRING
String toBase58(const uint8_t * array, size_t arraySize);
#endif
#if USE_STD_STRING
std::string toBase58(const uint8_t * array, size_t arraySize);
#endif


// base58 conversion with 4-byte checksum at the end (doubleSha)
size_t toBase58Check(const uint8_t * array, size_t arraySize, char * output, size_t outputSize);
#if USE_ARDUINO_STRING
String toBase58Check(const uint8_t * array, size_t arraySize);
#endif
#if USE_STD_STRING
std::string toBase58Check(const uint8_t * array, size_t arraySize);
#endif

size_t fromBase58Length(const char * array, size_t arraySize);
size_t fromBase58(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize);
size_t fromBase58Check(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize);
#if !(USE_ARDUINO_STRING || USE_STD_STRING)
size_t fromBase58(const char * encoded, uint8_t * array, size_t arraySize);
size_t fromBase58Check(const char * encoded, uint8_t * array, size_t arraySize);
#endif
#if USE_ARDUINO_STRING
size_t fromBase58(String encoded, uint8_t * output, size_t outputSize);
size_t fromBase58Check(String encoded, uint8_t * output, size_t outputSize);
#endif
#if USE_STD_STRING
size_t fromBase58(std::string encoded, uint8_t * output, size_t outputSize);
size_t fromBase58Check(std::string encoded, uint8_t * output, size_t outputSize);
#endif

size_t toBase43Length(const uint8_t * array, size_t arraySize);
size_t toBase43(const uint8_t * array, size_t arraySize, char * output, size_t outputSize);
#if USE_ARDUINO_STRING
String toBase43(const uint8_t * array, size_t arraySize);
#endif
#if USE_STD_STRING
std::string toBase43(const uint8_t * array, size_t arraySize);
#endif
size_t fromBase43Length(const char * array, size_t arraySize);
size_t fromBase43(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize);
#if !(USE_ARDUINO_STRING || USE_STD_STRING)
size_t fromBase43(const char * encoded, uint8_t * array, size_t arraySize);
#endif
#if USE_ARDUINO_STRING
size_t fromBase43(String encoded, uint8_t * output, size_t outputSize);
#endif
#if USE_STD_STRING
size_t fromBase43(std::string encoded, uint8_t * output, size_t outputSize);
#endif

size_t toHex(const uint8_t * array, size_t arraySize, char * output, size_t outputSize);
#if USE_ARDUINO_STRING
String toHex(const uint8_t * array, size_t arraySize);
size_t toHex(uint8_t v, Print &s); // printing single hex value to Print
size_t toHex(const uint8_t * array, size_t arraySize, Print &s); // TODO: pass pointer instead printing array in hex Print
#endif
#if USE_STD_STRING
std::string toHex(const uint8_t * array, size_t arraySize);
#endif
size_t fromHex(const char * hex, size_t hexLen, uint8_t * array, size_t arraySize);
uint8_t hexToVal(char c);
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromHex(const char * hex, uint8_t * array, size_t arraySize);
#endif
#if USE_ARDUINO_STRING
size_t fromHex(String encoded, uint8_t * output, size_t outputSize);
#endif
#if USE_STD_STRING
size_t fromHex(std::string encoded, uint8_t * output, size_t outputSize);
#endif

size_t toBin(const uint8_t * array, size_t arraySize, char * output, size_t outputSize);
#if USE_ARDUINO_STRING
String toBin(const uint8_t * array, size_t arraySize);
size_t toBin(uint8_t v, Print &s); // printing single value to Print
size_t toBin(const uint8_t * array, size_t arraySize, Print &s);
#endif
#if USE_STD_STRING
std::string toBin(const uint8_t * array, size_t arraySize);
#endif
size_t fromBin(const char * bin, size_t binLen, uint8_t * array, size_t arraySize);
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromBin(const char * hex, uint8_t * array, size_t arraySize);
#endif
#if USE_ARDUINO_STRING
size_t fromBin(String encoded, uint8_t * output, size_t outputSize);
#endif
#if USE_STD_STRING
size_t fromBin(std::string encoded, uint8_t * output, size_t outputSize);
#endif

size_t toBase64Length(const uint8_t * array, size_t arraySize, uint8_t flags = BASE64_STANDARD);
size_t toBase64(const uint8_t * array, size_t arraySize, char * output, size_t outputSize, uint8_t flags = BASE64_STANDARD);
#if USE_ARDUINO_STRING
String toBase64(const uint8_t * array, size_t arraySize, uint8_t flags = BASE64_STANDARD);
#endif
#if USE_STD_STRING
std::string toBase64(const uint8_t * array, size_t arraySize, uint8_t flags = BASE64_STANDARD);
#endif
size_t fromBase64Length(const char * array, size_t arraySize, uint8_t flags = BASE64_STANDARD);
size_t fromBase64(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize, uint8_t flags = BASE64_STANDARD);
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromBase64(const char * encoded, uint8_t * array, size_t arraySize, uint8_t flags = BASE64_STANDARD);
#endif
#if USE_ARDUINO_STRING
size_t fromBase64(String encoded, uint8_t * output, size_t outputSize, uint8_t flags = BASE64_STANDARD);
String base64ToHex(String b64, uint8_t flags = BASE64_STANDARD);
String hexToBase64(String hex, uint8_t flags = BASE64_STANDARD);
#endif
#if USE_STD_STRING
size_t fromBase64(std::string encoded, uint8_t * output, size_t outputSize, uint8_t flags = BASE64_STANDARD);
std::string base64ToHex(std::string b64, uint8_t flags = BASE64_STANDARD);
std::string hexToBase64(std::string hex, uint8_t flags = BASE64_STANDARD);
#endif

/* int conversion */
uint64_t littleEndianToInt(const uint8_t * array, size_t arraySize);
void intToLittleEndian(uint64_t num, uint8_t * array, size_t arraySize);
uint64_t bigEndianToInt(const uint8_t * array, size_t arraySize);
void intToBigEndian(uint64_t num, uint8_t * array, size_t arraySize);

/* varint */
uint8_t lenVarInt(uint64_t num); // returns length of the array required for varint encoding
uint64_t readVarInt(const uint8_t * array, size_t arraySize);
size_t writeVarInt(uint64_t num, uint8_t * array, size_t arraySize);


#endif
