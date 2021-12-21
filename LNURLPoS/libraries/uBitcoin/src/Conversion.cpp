#include "Conversion.h"
#include "Hash.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "utility/trezor/memzero.h"

#if USE_STD_STRING
using std::string;
#define String string
#endif

static const char BASE58_CHARS[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const char BASE43_CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$*+-./:";
static const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t toHex(const uint8_t * array, size_t arraySize, char * output, size_t outputSize){
    if(array == NULL || output == NULL){ return 0; }
    // uint8_t * array = (uint8_t *) arr;
    if(outputSize < 2*arraySize){
        return 0;
    }
    memzero(output, outputSize);

    for(size_t i=0; i < arraySize; i++){
        output[2*i] = (array[i] >> 4) + '0';
        if(output[2*i] > '9'){
            output[2*i] += 'a'-'9'-1;
        }

        output[2*i+1] = (array[i] & 0x0F) + '0';
        if(output[2*i+1] > '9'){
            output[2*i+1] += 'a'-'9'-1;
        }
    }
    return 2*arraySize;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String toHex(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return String(); }
    size_t outputSize = arraySize * 2 + 1;
    char * output = (char *) malloc(outputSize);
    if(output == NULL){ return String(); }

    toHex(array, arraySize, output, outputSize);

    String result(output);

    memzero(output, outputSize);
    free(output);

    return result;
}
#endif
#if USE_ARDUINO_STRING
size_t toHex(uint8_t v, Print &s){
    char c = (v >> 4) + '0';
    if(c > '9'){
        c += 'a'-'9'-1;
    }
    s.print(c);

    c = (v & 0x0F) + '0';
    if(c > '9'){
        c += 'a'-'9'-1;
    }
    s.print(c);
    return 2;
}

size_t toHex(const uint8_t * array, size_t arraySize, Print &s){
    if(array == NULL){ return 0; }
    size_t l = 0;
    for(int i=0; i<arraySize; i++){
        l += toHex(array[i], s);
    }
    return l;
}
#endif

uint8_t hexToVal(char c){
    if(c >= '0' && c <= '9'){
        return ((uint8_t)(c - '0')) & 0x0F;
    }
    if(c >= 'A' && c <= 'F'){
        return ((uint8_t)(c-'A'+10)) & 0x0F;
    }
    if(c >= 'a' && c <= 'f'){
        return ((uint8_t)(c-'a'+10)) & 0x0F;
    }
    return 0xFF;
}

size_t fromHex(const char * hex, size_t hexLen, uint8_t * array, size_t arraySize){
    if(array == NULL || hex == NULL){ return 0; }
    memzero(array, arraySize);
    // ignoring all non-hex characters in the beginning
    size_t offset = 0;
    while(offset < hexLen){
        uint8_t v = hexToVal(hex[offset]);
        if(v > 0x0F){ // if invalid char
            offset++;
        }else{
            break;
        }
    }
    hexLen -= offset;
    for(size_t i=0; i<hexLen/2; i++){
        uint8_t v1 = hexToVal(hex[offset+2*i]);
        uint8_t v2 = hexToVal(hex[offset+2*i+1]);
        if((v1 > 0x0F) || (v2 > 0x0F)){ // if invalid char stop parsing
            return i;
        }
        array[i] = (v1<<4) | v2;
    }
    return hexLen/2;
}
#if USE_STD_STRING || USE_ARDUINO_STRING
size_t fromHex(String encoded, uint8_t * output, size_t outputSize){
    if(output == NULL){ return 0; }
    return fromHex(encoded.c_str(), encoded.length(), output, outputSize);
};
#endif
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromHex(const char * hex, uint8_t * array, size_t arraySize){
    if(array == NULL || hex == NULL){ return 0; }
    return fromHex(hex, strlen(hex), array, arraySize);
}
#endif

/******************** Binary conversion *******************/

size_t fromBin(const char * bin, size_t binLen, uint8_t * array, size_t arraySize){
    if(bin == NULL || array == NULL){ return 0; }
    // array is big enough
    if(arraySize*8 < binLen){
        return 0;
    }
    size_t len = binLen/8;
    if(binLen % 8 != 0){
        len += 1; // not aligned to 8 bits
    }
    // zero output array
    memzero(array, arraySize);
    for(size_t i = 0; i < binLen; i++){
        // we go in reverse order (from the end of the string)
        uint8_t exp = (i%8); // shift
        uint8_t n = (i/8); // current byte from the end
        char c = bin[binLen-i-1];
        if(c == '1'){
            // set bit
            array[len-n-1] |= (1<<exp);
        }else if(c == '0'){
            // correct char, nothing to do here
        }else{
            // wrong char
            return 0;
        }
    }
    return len;
}

size_t toBin(const uint8_t * array, size_t arraySize, char * output, size_t outputSize){
  if(array == NULL || output == NULL){ return 0; }
  if(outputSize < arraySize*8){
    return 0;
  }
  memzero(output, outputSize);
  for(size_t i=0; i<arraySize; i++){
    for(size_t j=0; j<8; j++){
      if(((array[i] >> j) & 1) == 1){
        output[8*i+(7-j)] = '1';
      }else{
        output[8*i+(7-j)] = '0';
      }
    }
  }
  return 8*arraySize;
}

#if USE_ARDUINO_STRING || USE_STD_STRING
String toBin(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return String(); }
    size_t outputSize = arraySize * 8 + 1;
    char * output = (char *) malloc(outputSize);
    if(output == NULL){ return String(); }

    toBin(array, arraySize, output, outputSize);

    String result(output);

    memzero(output, outputSize);
    free(output);

    return result;
}
#endif
#if USE_ARDUINO_STRING
size_t toBin(uint8_t v, Print &s){
    for(int i=7; i>=0; i--){
        if(((v>>i) & 1)==1){
            s.print('1');
        }else{
            s.print('0');
        }
    }
    return 8;
}

size_t toBin(const uint8_t * array, size_t arraySize, Print &s){
    if(array == NULL){ return 0; }
    size_t l = 0;
    for(int i=0; i<arraySize; i++){
        l += toBin(array[i], s);
    }
    return l;
}
#endif

#if USE_STD_STRING || USE_ARDUINO_STRING
size_t fromBin(String encoded, uint8_t * output, size_t outputSize){
    if(output == NULL){ return 0; }
    return fromBin(encoded.c_str(), encoded.length(), output, outputSize);
};
#endif
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromBin(const char * hex, uint8_t * array, size_t arraySize){
    if(hex == NULL || array == NULL){ return 0; }
    return fromBin(hex, strlen(hex), array, arraySize);
}
#endif
/******************* Base 58 conversion *******************/

size_t toBase58Length(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return 0; }
    // Counting leading zeroes
    size_t zeroCount = 0;
    while(zeroCount < arraySize && !array[zeroCount]){
        zeroCount++;
    }

    /*
     *  Encoded string will have maximum length of
     *  len = arraySize * log(58)/log(256) ≈ arraySize * 1.37
     *  and should be rounded to larger value
     *  Limitation: size_t overflow when arraySize > 65536/56 = 1170 bytes
     *  Extra byte due to numerical error appears after 5117 bytes
     */

    size_t size = (arraySize - zeroCount) * 183 / 134 + 1;
    // size_t size = (arraySize - zeroCount) * 137 / 100 + 1;
    return size+zeroCount;
}

size_t toBase58(const uint8_t * array, size_t arraySize, char * output, size_t outputSize){
    if(array == NULL || output == NULL){ return 0; }
    // Counting leading zeroes
    size_t zeroCount = 0;
    while(zeroCount < arraySize && !array[zeroCount]){
        zeroCount++;
    }

    // TODO: refactor with real sizes
    // size estimation. 56/41 ≈ log(58)/log(256)
    size_t size = (arraySize - zeroCount) * 183 / 134 + 1;
    // size_t size = (arraySize - zeroCount) * 137 / 100 + 1;
    if(outputSize < size+zeroCount){
        return 0;
    }

    memzero(output, outputSize);

    // array copy for manipulations
    size_t bufferSize = arraySize - zeroCount;
    uint8_t * buffer = (uint8_t *)calloc(bufferSize, sizeof(uint8_t));
    if(buffer == NULL){ return 0; }
    for(size_t i = zeroCount; i < arraySize; i++){
        buffer[i - zeroCount] = array[i];
    }

    for(size_t j = 0; j < size; j++){
        uint16_t reminder = 0;
        uint16_t temp = 0;
        for(size_t i = 0; i < bufferSize; i++){
            temp = (reminder * 256 + buffer[i]);
            reminder = temp % 58;
            buffer[i] = temp/58;
        }
        output[size+zeroCount-j-1] = BASE58_CHARS[reminder];
    }
    free(buffer);
    for (size_t i = 0; i < zeroCount; i++){
        output[i] = BASE58_CHARS[0];
    }
    if(outputSize > size+zeroCount){
        output[size+zeroCount] = '\0';
    }

    // removing leading zeroes
    // TODO: refactor
    int shift = 0;
    for(size_t i=zeroCount; i < size+zeroCount; i++){
        if(output[i]==BASE58_CHARS[0]){
            shift++;
        }else{
            break;
        }
    }
    if(shift>0){
        for(size_t i=zeroCount+shift; i < size+zeroCount; i++){
            output[i-shift] = output[i];
            output[i] = 0;
        }
    }
    size_t l = size+zeroCount-shift;
    memzero(output + l, outputSize-l);
    return l;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String toBase58(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return String(); }
    size_t len = toBase58Length(array, arraySize) + 1; // +1 for null terminator
    char * buf = (char *)malloc(len);
    if(buf == NULL){ return String(); }
    toBase58(array, arraySize, buf, len);
    String result(buf);
    free(buf);
    return result;
}
#endif

size_t toBase58Check(const uint8_t * array, size_t arraySize, char * output, size_t outputSize){
    if(array == NULL || output == NULL){ return 0; }
    uint8_t * arr = (uint8_t *) malloc(arraySize+4);
    if(arr == NULL){ return 0; }
    memcpy(arr, array, arraySize);

    uint8_t hash[32];
    doubleSha(arr, arraySize, hash);
    memcpy(arr+arraySize, hash, 4);

    size_t l = toBase58(arr, arraySize+4, output, outputSize);
    memzero(arr, arraySize+4); // secret should not stay in RAM
    free(arr);
    return l;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String toBase58Check(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return String(); }
    size_t len = toBase58Length(array, arraySize + 4) + 1; // +4 checksum +1 for null terminator
    char * buf = (char *)malloc(len);
    if(buf == NULL){ return String(); }
    toBase58Check(array, arraySize, buf, len);
    String result(buf);
    free(buf);
    return result;
}
#endif


// TODO: add zero count, fix wrong length
size_t fromBase58Length(const char * array, size_t arraySize){
    if(array == NULL){ return 0; }
    size_t size = arraySize * 361 / 493 + 1;
    return size;
}

size_t fromBase58(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize){
    if(encoded == NULL || output == NULL){ return 0; }
    memzero(output, outputSize);

    size_t l;
    // looking for the end of char array
    for(l=0; l<encodedSize; l++){
        const char * pch = strchr(BASE58_CHARS, encoded[l]);
        if(pch==NULL){ // char not in the alphabet
            break;
        }
    }
    encodedSize = l;
    size_t size = fromBase58Length(encoded, encodedSize);
    uint8_t * tmp = (uint8_t *) calloc(size, sizeof(uint8_t));
    if(tmp == NULL){ return 0; }

    uint8_t zeroCount = 0;
    for(size_t i = 0; i<encodedSize; i++){
        if(encoded[i] == BASE58_CHARS[0]){
            zeroCount++;
        }else{
            break;
        }
    }

    uint16_t val = 0;
    for(size_t i = 0; i < encodedSize; i++){
        const char * pch = strchr(BASE58_CHARS, encoded[i]);
        if(pch!=NULL){
            val = pch - BASE58_CHARS;
            for(size_t j = 0; j < size; j++){
                uint16_t cur = tmp[size-j-1]*58;
                cur += val;
                val = cur/256;
                tmp[size-j-1] = cur%256;
            }
        }else{
            free(tmp);
            return 0;
        }
    }
    // shifting array
    uint8_t shift = 0;
    for(size_t i = zeroCount; i < size; i++){
        if(tmp[i] == 0){
            shift++;
        }else{
            break;
        }
    }
    if(size-shift > outputSize){
        free(tmp);
        return 0;
    }
    memcpy(output, tmp+shift, size-shift);
    free(tmp);
    return size-shift;
}

size_t fromBase58Check(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize){
    if(encoded == NULL || output == NULL){ return 0; }
    uint8_t * arr = (uint8_t *) malloc(outputSize+4);
    if(arr == NULL){ return 0; }
    size_t l = fromBase58(encoded, encodedSize, arr, outputSize+4);
    if(l<4){
        free(arr);
        return 0;
    }

    uint8_t hash[32];
    doubleSha(arr, l-4, hash);
    if(memcmp(arr+l-4, hash, 4)!=0){
        free(arr);
        return 0;
    }

    memcpy(output, arr, l-4);

    memzero(arr, outputSize+4); // secret should not stay in RAM
    free(arr);
    return l-4;
}

#if USE_STD_STRING || USE_ARDUINO_STRING
size_t fromBase58(String encoded, uint8_t * output, size_t outputSize){
    return fromBase58(encoded.c_str(), encoded.length(), output, outputSize);
};
size_t fromBase58Check(String encoded, uint8_t * output, size_t outputSize){
    return fromBase58Check(encoded.c_str(), encoded.length(), output, outputSize);
};
#endif
#if !(USE_ARDUINO_STRING || USE_STD_STRING)
size_t fromBase58(const char * encoded, uint8_t * array, size_t arraySize){
    return fromBase58(encoded, strlen(encoded), array, arraySize);
}
size_t fromBase58Check(const char * encoded, uint8_t * array, size_t arraySize){
    return fromBase58Check(encoded, strlen(encoded), array, arraySize);
}
#endif

/******************* Base 43 conversion *******************/

size_t toBase43Length(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return 0; }
    // Counting leading zeroes
    size_t zeroCount = 0;
    while(zeroCount < arraySize && !array[zeroCount]){
        zeroCount++;
    }
    // size estimation. log(256)/log(43)
    size_t size = (arraySize - zeroCount) * 148 / 100 + 1;
    return size+zeroCount;
}

size_t toBase43(const uint8_t * array, size_t arraySize, char * output, size_t outputSize){
    if(array == NULL || output == NULL){ return 0; }
    // Counting leading zeroes
    size_t zeroCount = 0;
    while(zeroCount < arraySize && !array[zeroCount]){
        zeroCount++;
    }

    // TODO: refactor with real sizes
    // size estimation. log(256)/log(43)
    size_t size = (arraySize - zeroCount) * 148 / 100 + 1;
    if(outputSize < size+zeroCount){
        return 0;
    }

    memzero(output, outputSize);

    // array copy for manipulations
    size_t bufferSize = arraySize - zeroCount;
    uint8_t * buffer = (uint8_t *)calloc(bufferSize, sizeof(uint8_t));
    if(buffer == NULL){ return 0; }
    for(size_t i = zeroCount; i < arraySize; i++){
        buffer[i - zeroCount] = array[i];
    }

    for(size_t j = 0; j < size; j++){
        uint16_t reminder = 0;
        uint16_t temp = 0;
        for(size_t i = 0; i < bufferSize; i++){
            temp = (reminder * 256 + buffer[i]);
            reminder = temp % 43;
            buffer[i] = temp/43;
        }
        output[size+zeroCount-j-1] = BASE43_CHARS[reminder];
    }
    free(buffer);
    for (size_t i = 0; i < zeroCount; i++){
        output[i] = BASE43_CHARS[0];
    }
    if(outputSize > size+zeroCount){
        output[size+zeroCount] = '\0';
    }

    // removing leading zeroes
    // TODO: refactor
    int shift = 0;
    for(size_t i=zeroCount; i < size+zeroCount; i++){
        if(output[i]==BASE43_CHARS[0]){
            shift++;
        }else{
            break;
        }
    }
    if(shift>0){
        for(size_t i=zeroCount+shift; i < size+zeroCount; i++){
            output[i-shift] = output[i];
            output[i] = 0;
        }
    }
    size_t l = size+zeroCount-shift;
    memzero(output + l, outputSize-l);
    return l;
}
#if (USE_STD_STRING || USE_ARDUINO_STRING)
String toBase43(const uint8_t * array, size_t arraySize){
    if(array == NULL){ return String(); }
    size_t l = toBase43Length(array, arraySize);
    char * output = (char *)calloc(l+1, sizeof(char));
    if(output == NULL){ return String(); }
    toBase43(array, arraySize, output, l);
    String s(output);
    free(output);
    return s;
}
#endif
// TODO: add zero count, fix wrong length
size_t fromBase43Length(const char * array, size_t arraySize){
    if(array == NULL){ return 0; }
    size_t size = arraySize * 68 / 100 + 1;
    return size;
}

size_t fromBase43(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize){
    if(encoded == NULL || output == NULL){ return 0; }
    memzero(output, outputSize);

    size_t l;
    // looking for the end of char array
    for(l=0; l<encodedSize; l++){
        const char * pch = strchr(BASE43_CHARS, encoded[l]);
        if(pch==NULL){ // char not in the alphabet
            break;
        }
    }
    encodedSize = l;
    size_t size = fromBase43Length(encoded, encodedSize);
    uint8_t * tmp = (uint8_t *) calloc(size, sizeof(uint8_t));
    if(tmp == NULL){ return 0; }

    uint8_t zeroCount = 0;
    for(size_t i = 0; i<encodedSize; i++){
        if(encoded[i] == BASE43_CHARS[0]){
            zeroCount++;
        }else{
            break;
        }
    }

    uint16_t val = 0;
    for(size_t i = 0; i < encodedSize; i++){
        const char * pch = strchr(BASE43_CHARS, encoded[i]);
        if(pch!=NULL){
            val = pch - BASE43_CHARS;
            for(size_t j = 0; j < size; j++){
                uint16_t cur = tmp[size-j-1]*43;
                cur += val;
                val = cur/256;
                tmp[size-j-1] = cur%256;
            }
        }else{
            free(tmp);
            return 0;
        }
    }
    // shifting array
    uint8_t shift = 0;
    for(size_t i = zeroCount; i < size; i++){
        if(tmp[i] == 0){
            shift++;
        }else{
            break;
        }
    }
    if(size-shift > outputSize){
        free(tmp);
        return 0;
    }
    memcpy(output, tmp+shift, size-shift);
    free(tmp);
    return size-shift;
}
#if (USE_STD_STRING || USE_ARDUINO_STRING)
size_t fromBase43(String encoded, uint8_t * output, size_t outputSize){
    return fromBase43(encoded.c_str(), encoded.length(), output, outputSize);
};
#endif
#if !(USE_ARDUINO_STRING || USE_STD_STRING)
size_t fromBase43(const char * encoded, uint8_t * array, size_t arraySize){
    return fromBase43(encoded, strlen(encoded), array, arraySize);
}
#endif

/******************* Base 64 conversion *******************/

size_t toBase64Length(const uint8_t * array, size_t arraySize, uint8_t flags){
    if(array == NULL){ return 0; }
    size_t v = (arraySize / 3) * 4;
    if(arraySize % 3 != 0){
        if(flags & BASE64_NOPADDING){
            v += (arraySize % 3) + 1;
        }else{
            v += 4;
        }
    }
    return v;
}
size_t toBase64(const uint8_t * array, size_t arraySize, char * output, size_t outputSize, uint8_t flags){
    if(array == NULL || output == NULL){ return 0; }
    memzero(output, outputSize);
    size_t cur = 0;
    if(outputSize < toBase64Length(array, arraySize, flags)){
        return 0;
    }
    while(3 * cur + 3 < arraySize){
        uint32_t val = bigEndianToInt(array+3*cur, 3);
        for(uint8_t i=0; i<4; i++){
            output[4*cur + i] = BASE64_CHARS[((val >> (6*(3-i))) & 0x3F)];
        }
        cur++;
    }
    size_t len = cur * 4;
    if(arraySize % 3 != 0){
        uint8_t rem = arraySize % 3;
        uint32_t val = bigEndianToInt(array+3*cur, rem);
        val = val << ((3-rem) * 8);
        for(uint8_t i=0; i<(rem+1); i++){
            output[4*cur + i] = BASE64_CHARS[((val >> (6*(3-i))) & 0x3F)];
        }
        if(flags & BASE64_NOPADDING){
            len += (rem+1);
        }else{
            memset(output + 4 * cur + 1 + rem, '=', 3-rem);
            len += 4;
        }
    }else{
        uint32_t val = bigEndianToInt(array+3*cur, 3);
        for(uint8_t i=0; i<4; i++){
            output[4*cur + i] = BASE64_CHARS[((val >> (6*(3-i))) & 0x3F)];
        }
        len += 4;
    }
    // replace with urlsafe characters
    if(flags & BASE64_URLSAFE){
        for(size_t i=0; i<len; i++){
            if(output[i] == '+'){
                output[i] = '-';
            }
            if(output[i] == '/'){
                output[i] = '_';
            }
        }
    }
    return len;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String toBase64(const uint8_t * array, size_t arraySize, uint8_t flags){
    if(array == NULL){ return String(); }
    size_t len = toBase64Length(array, arraySize, flags) + 1; // +1 for null terminator
    char * buf = (char *)malloc(len);
    if(buf==NULL){ return String(); }
    toBase64(array, arraySize, buf, len, flags);
    String result(buf);
    free(buf);
    return result;
}
#endif
size_t fromBase64Length(const char * array, size_t arraySize, uint8_t flags){
    if(array == NULL){ return 0; }
    if(arraySize % 4 != 0 && (flags & BASE64_NOPADDING) == 0){ return 0; }
    size_t v = (arraySize / 4) * 3;
    if(flags & BASE64_NOPADDING){
        if(arraySize % 4 != 0){
            v += (arraySize % 4)-1;
        }
    }else{
        if(array[arraySize-1] == '='){
            v--;
        }
        if(array[arraySize-2] == '='){
            v--;
        }
    }
    return v;
}
size_t fromBase64(const char * encoded, size_t encodedSize, uint8_t * output, size_t outputSize, uint8_t flags){
    if(encoded == NULL || output == NULL){ return 0; }
    size_t cur = 0;
    memzero(output, outputSize);
    if(outputSize < fromBase64Length(encoded, encodedSize, flags)){
        return 0;
    }
    while(cur*4 < encodedSize){
        if(cur*4+3 >= encodedSize && (flags & BASE64_NOPADDING) == 0){
            memzero(output, outputSize);
            return 0;
        }
        uint32_t val = 0;
        for(size_t i=0; i<4; i++){
            char c = encoded[cur*4+i];
            // replace characters for urlsafe version
            if(c=='-' && (flags & BASE64_URLSAFE)){
                c = '+';
            }
            if(c=='_' && (flags & BASE64_URLSAFE)){
                c = '/';
            }
            const char * pch = strchr(BASE64_CHARS, c);
            if(pch==NULL || ((flags & BASE64_NOPADDING) && (c == 0))){
                if((encoded[cur*4+i] == '=') || (c == 0 && (flags & BASE64_NOPADDING))){
                    if(i==3){
                        val = (val >> 2);
                        if(outputSize < 3*cur+2){
                            memzero(output, outputSize);
                            return 0;
                        }
                        intToBigEndian(val, output+3*cur, 2);
                        return 3*cur + 2;
                    }
                    if(i==2){
                        val = (val >> 4);
                        if(outputSize < 3*cur+1){
                            memzero(output, outputSize);
                            return 0;
                        }
                        output[3*cur] = (val & 0xFF);
                        return 3*cur + 1;
                    }
                }
                memzero(output, outputSize);
                return 0;
            }else{
                val = (val << 6) + ((pch - BASE64_CHARS) & 0x3F);
            }
        }
        if(outputSize < 3*(cur+1)){
            memzero(output, outputSize);
            printf("6 %lu < 3* %lu+1\n", outputSize, cur);
            return 0;
        }
        intToBigEndian(val, output+3*cur, 3);
        cur++;
    }
    return 3 * cur;
}
#if USE_STD_STRING || USE_ARDUINO_STRING
size_t fromBase64(String encoded, uint8_t * output, size_t outputSize, uint8_t flags){
    return fromBase64(encoded.c_str(), encoded.length(), output, outputSize, flags);
};
String base64ToHex(String b64, uint8_t flags){
    size_t len = fromBase64Length(b64.c_str(), strlen(b64.c_str()), flags) + 1; // +1 for null terminator
    uint8_t * buf = (uint8_t *)malloc(len);
    if(buf==NULL){ return String(); }
    len = fromBase64(b64, buf, len, flags);
    String result = toHex(buf, len);
    free(buf);
    return result;
};
String hexToBase64(String hex, uint8_t flags){
    size_t len = strlen(hex.c_str())/2+1; // +1 for null terminator
    uint8_t * buf = (uint8_t *)malloc(len);
    if(buf==NULL){ return String(); }
    len = fromHex(hex, buf, len);
    String result = toBase64(buf, len, flags);
    free(buf);
    return result;
};
#endif
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
size_t fromBase64(const char * b64, uint8_t * array, size_t arraySize, uint8_t flags){
    if(b64 == NULL || array == NULL){ return 0; }
    return fromBase64(b64, strlen(b64), array, arraySize, flags);
}
#endif

/* Integer conversion */

uint64_t littleEndianToInt(const uint8_t * array, size_t arraySize){
    uint64_t num = 0;
    for(size_t i = 0; i < arraySize; i++){
        num <<= 8;
        num += (array[arraySize-i-1] & 0xFF);
    }
    return num;
}

void intToLittleEndian(uint64_t num, uint8_t * array, size_t arraySize){
    for(size_t i = 0; i < arraySize; i++){
        array[i] = ((num >> (8*i)) & 0xFF);
    }
}

uint64_t bigEndianToInt(const uint8_t * array, size_t arraySize){
    uint64_t num = 0;
    for(size_t i = 0; i < arraySize; i++){
        num <<= 8;
        num += (array[i] & 0xFF);
    }
    return num;
}

void intToBigEndian(uint64_t num, uint8_t * array, size_t arraySize){
    for(size_t i = 0; i < arraySize; i++){
        array[arraySize-i-1] = ((num >> (8*i)) & 0xFF);
    }
}

/* Varint */

uint8_t lenVarInt(uint64_t num){
    if(num < 0xfd){
        return 1;
    }
    if((num >> 16) == 0){
        return 3;
    }
    if((num >> 32) == 0){
        return 5;
    }
    return 9;
}
uint64_t readVarInt(const uint8_t * array, size_t arraySize){
    if(array[0] < 0xfd){
        return array[0];
    }else{
        size_t len = (1 << (array[0] - 0xfc));
        if(len+1 > arraySize){
            return 0;
        }
        return littleEndianToInt(array + 1, len);
    }
}

// TODO: don't repeat yourself!
size_t writeVarInt(uint64_t num, uint8_t * array, size_t arraySize){
    uint8_t len = lenVarInt(num);
    if(arraySize < len){
        return 0;
    }
    if(len == 1){
        array[0] = (uint8_t)(num & 0xFF);
    }else{
        switch(len){
            case 3: array[0] = 0xfd;
                    break;
            case 5: array[0] = 0xfe;
                    break;
            case 9: array[0] = 0xff;
                    break;
        }
        intToLittleEndian(num, array+1, len-1);
    }
    return len;
}

