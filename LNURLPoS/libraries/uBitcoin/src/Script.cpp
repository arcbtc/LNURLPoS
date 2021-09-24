#include "Bitcoin.h"
#include "Hash.h"
#include "Conversion.h"
#include "OpCodes.h"
#include "utility/segwit_addr.h"

#define MAX_SCRIPT_SIZE 10000

//------------------------------------------------------------ Script
void Script::init(){
    reset();
    scriptLen = 0;
    scriptArray = NULL;    
}
Script::Script(void){
    init();
}
Script::Script(const uint8_t * buffer, size_t len){
    reset();
    push(buffer, len);
}
void Script::fromAddress(const char * address){
    reset();
    scriptLen = 0;
    scriptArray = NULL;
    uint8_t addr[21];
    size_t len = strlen(address);
    if(len > 100){ // very wrong address
        return;
    }
    ScriptType type = UNKNOWN_TYPE;
    const Network * network;
    for(int i=0; i<networks_len; i++){
        if(memcmp(address, networks[i]->bech32, strlen(networks[i]->bech32))==0){
            type = P2WPKH;
            network = networks[i];
            break;
        }
    }
    // segwit
    if(type == P2WPKH){
        int ver = 0;
        uint8_t prog[32];
        size_t prog_len = 0;
        int r = segwit_addr_decode(&ver, prog, &prog_len, network->bech32, address);
        if(r != 1){ // decoding failed
            return;
        }
        scriptLen = prog_len + 2;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
        scriptArray[0] = ver;
        scriptArray[1] = prog_len; // varint?
        memcpy(scriptArray+2, prog, prog_len);
    }else{ // legacy or nested segwit
        int l = fromBase58Check(address, len, addr, sizeof(addr));
        if(l != 21){ // either wrong checksum or wierd address
            return;
        }
        for(int i=0; i<networks_len; i++){
            if(addr[0]==networks[i]->p2pkh){
                type = P2PKH;
                network = networks[i];
                break;
            }
            if(addr[0]==networks[i]->p2sh){
                type = P2SH;
                network = networks[i];
                break;
            }
        }
        if(type == P2PKH){
            scriptLen = 25;
            scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
            scriptArray[0] = OP_DUP;
            scriptArray[1] = OP_HASH160;
            scriptArray[2] = 20;
            memcpy(scriptArray+3, addr+1, 20);
            scriptArray[23] = OP_EQUALVERIFY;
            scriptArray[24] = OP_CHECKSIG;
        }
        if(type == P2SH){
            scriptLen = 23;
            scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
            scriptArray[0] = OP_HASH160;
            scriptArray[1] = 20;
            memcpy(scriptArray+2, addr+1, 20);
            scriptArray[22] = OP_EQUAL;
        }
    }
}
Script::Script(const PublicKey pubkey, ScriptType type){
    reset();
    if(type == P2PKH){
        scriptLen = 25;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
        scriptArray[0] = OP_DUP;
        scriptArray[1] = OP_HASH160;
        scriptArray[2] = 20;
        uint8_t sec_arr[65] = { 0 };
        int l = pubkey.sec(sec_arr, sizeof(sec_arr));
        hash160(sec_arr, l, scriptArray+3);
        scriptArray[23] = OP_EQUALVERIFY;
        scriptArray[24] = OP_CHECKSIG;
    }
    if(type == P2WPKH){
        scriptLen = 22;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
        scriptArray[0] = 0x00;
        scriptArray[1] = 20;
        uint8_t sec_arr[65] = { 0 };
        int l = pubkey.sec(sec_arr, sizeof(sec_arr));
        hash160(sec_arr, l, scriptArray+2);
    }
}
Script::Script(const Script &other, ScriptType type){
    reset();
    if(type == P2SH){
        scriptLen = 23;
        scriptArray = (uint8_t *) calloc(scriptLen, sizeof(uint8_t));
        hash160(other.scriptArray, other.scriptLen, scriptArray+2);
        scriptArray[0] = OP_HASH160;
        scriptArray[1] = 20;
        scriptArray[scriptLen-1] = OP_EQUAL;
    }
    if(type == P2WSH){
        scriptLen = 34;
        scriptArray = (uint8_t *) calloc(scriptLen, sizeof(uint8_t));
        sha256(other.scriptArray, other.scriptLen, scriptArray+2);
        scriptArray[0] = 0x00;
        scriptArray[1] = 32;
    }
}
void Script::clear(){
    if(scriptLen > 0){
        free(scriptArray);
        scriptLen = 0;
    }
}
size_t Script::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        clear();
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    // reading scriptLen varint
    if(s->available() && bytes_parsed == 0){ 
        lenLen = s->read();
        bytes_read++;
        if(lenLen < 0xfd){
            scriptLen = lenLen;
            if(scriptLen > 0){
                scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
            }
            lenLen = 1;
        }else{
            scriptLen = 0;
            lenLen = 1+(1 << (lenLen - 0xfc));
            scriptArray = (uint8_t *) calloc( 255, sizeof(uint8_t));
        }
    }
    while(s->available() > 0 && bytes_parsed+bytes_read < lenLen){
        scriptLen += (s->read() << (8*(bytes_parsed+bytes_read-1)));
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == lenLen && scriptLen > 0){
        free(scriptArray);
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
    }
    if(bytes_parsed+bytes_read == lenLen && lenVarInt(scriptLen) != lenLen){
        status = PARSING_FAILED;
        bytes_parsed+=bytes_read;
        return bytes_read;
    }
    // reading the script
    while(s->available() > 0 && bytes_parsed+bytes_read < scriptLen+lenLen){
        scriptArray[bytes_parsed+bytes_read-lenLen] = s->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == scriptLen+lenLen){
        status = PARSING_DONE;
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}
size_t Script::to_stream(SerializeStream *s, size_t offset) const{
    size_t bytes_written = 0;
    // varint
    uint8_t l = lenVarInt(scriptLen);
    uint8_t arr[10];
    writeVarInt(scriptLen, arr, sizeof(arr));
    while(s->available() && bytes_written+offset < l){
        s->write(arr[offset+bytes_written]);
        bytes_written++;
    }
    while(s->available() && bytes_written+offset < l+scriptLen){
        s->write(scriptArray[bytes_written+offset-l]);
        bytes_written++;
    }
    return bytes_written;
}
ScriptType Script::type() const{
    if(
        (scriptLen == 25) &&
        (scriptArray[0] == OP_DUP) &&
        (scriptArray[1] == OP_HASH160) &&
        (scriptArray[2] == 20) &&
        (scriptArray[23] == OP_EQUALVERIFY) &&
        (scriptArray[24] == OP_CHECKSIG)
    ){
        return P2PKH;
    }
    if(
        (scriptLen == 23) &&
        (scriptArray[0] == OP_HASH160) &&
        (scriptArray[1] == 20) &&
        (scriptArray[22] == OP_EQUAL)
    ){
        return P2SH;
    }
    if(
        (scriptLen == 22) &&
        (scriptArray[0] == 0x00) &&
        (scriptArray[1] == 20)
    ){
        return P2WPKH;
    }
    if(
        (scriptLen == 34) &&
        (scriptArray[0] == 0x00) &&
        (scriptArray[1] == 32)
    ){
        return P2WSH;
    }
    return UNKNOWN_TYPE;
}
size_t Script::address(char * buffer, size_t len, const Network * network) const{
    memset(buffer, 0, len);
    if(type() == P2PKH){
        uint8_t addr[21];
        addr[0] = network->p2pkh;
        memcpy(addr+1, scriptArray + 3, 20);
        char address[40] = { 0 };
        toBase58Check(addr, 21, address, sizeof(address));
        size_t l = strlen(address);
        if(l > len){
            return 0;
        }
        memcpy(buffer, address, l);
        return l;
    }
    if(type() == P2SH){
        uint8_t addr[21];
        addr[0] = network->p2sh;
        memcpy(addr+1, scriptArray + 2, 20);
        char address[40] = { 0 };
        toBase58Check(addr, 21, address, sizeof(address));
        size_t l = strlen(address);
        if(l > len){
            return 0;
        }
        memcpy(buffer, address, l);
        return l;
    }
    if(type() == P2WPKH || type() == P2WSH){
        char address[76] = { 0 };
        segwit_addr_encode(address, network->bech32, scriptArray[0], scriptArray+2, scriptArray[1]);
        size_t l = strlen(address);
        if(l > len){
            return 0;
        }
        memcpy(buffer, address, l);
        return l;
    }
    return 0;
}
#if USE_ARDUINO_STRING
String Script::address(const Network * network) const{
    char buffer[100] = { 0 };
    size_t l = address(buffer, sizeof(buffer), network);
    if(l == 0){
        return String("");
    }
    return String(buffer);
}
#endif
#if USE_STD_STRING
std::string Script::address(const Network * network) const{
    char buffer[100] = { 0 };
    size_t l = address(buffer, sizeof(buffer), network);
    if(l == 0){
        return string("");
    }
    return string(buffer);
}
#endif
size_t Script::length() const{
    return scriptLen + lenVarInt(scriptLen);
}
size_t Script::push(uint8_t code){
    if(scriptLen+1 > MAX_SCRIPT_SIZE){
        clear();
        return 0;
    }
    if(scriptLen == 0){
        scriptLen = 1;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
    }else{
        scriptLen ++;
        scriptArray = (uint8_t *) realloc( scriptArray, scriptLen * sizeof(uint8_t));
    }
    scriptArray[scriptLen-1] = code;
    return scriptLen;
}
size_t Script::push(const uint8_t * data, size_t len){
    if(scriptLen+len > MAX_SCRIPT_SIZE){
        clear();
        return 0;
    }
    if(scriptLen == 0){
        scriptArray = (uint8_t *) calloc( len, sizeof(uint8_t));
    }else{
        scriptArray = (uint8_t *) realloc( scriptArray, (scriptLen + len) * sizeof(uint8_t));
    }
    memcpy(scriptArray + scriptLen, data, len);
    scriptLen += len;
    return scriptLen;
}
size_t Script::push(const PublicKey pubkey){
    uint8_t sec[65];
    uint8_t len = pubkey.sec(sec, sizeof(sec));
    push(len);
    push(sec, len);
    return scriptLen;
}
size_t Script::push(const Signature sig, SigHashType sigType){
    uint8_t der[75];
    uint8_t len = sig.der(der, sizeof(der));
    push(len+1);
    push(der, len);
    push(sigType);
    return scriptLen;
}
size_t Script::push(const Script sc){
    uint8_t len = sc.length();
    uint8_t * tmp;
    tmp = (uint8_t *)calloc(len, sizeof(uint8_t));
    sc.serialize(tmp, len);
    push(tmp, len);
    return scriptLen;
}
Script Script::scriptPubkey(ScriptType type) const{
    Script sc(*this, type);
    return sc;
}
Script &Script::operator=(const Script &other){
    reset();
    clear();
    if(other.scriptLen > 0){
        scriptLen = other.scriptLen;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
        memcpy(scriptArray, other.scriptArray, scriptLen);
    }
    return *this;
};
Script::Script(const Script &other){
    init();
    if(other.scriptLen > 0){
        scriptLen = other.scriptLen;
        scriptArray = (uint8_t *) calloc( scriptLen, sizeof(uint8_t));
        memcpy(scriptArray, other.scriptArray, scriptLen);
    }
};

//------------------------------------------------------------ Witness

void Witness::clear(){
    numElements = 0;
    if(witnessLen > 0){
        free(witnessArray);
        witnessLen = 0;
    }
}
void Witness::init(){
    numElements = 0;
    witnessLen = 0;
    reset();
    clear();
}
Witness::Witness(void){
    init();
}
Witness::Witness(const uint8_t * buffer, size_t len){
    numElements = 0;
    witnessLen = 0;
    reset();
    clear();
    ParseByteStream s(buffer, len);
    Witness::from_stream(&s);
}
Witness::Witness(const Signature sig, const PublicKey pubkey){
    numElements = 0;
    witnessLen = 0;
    reset();
    clear();
    push(sig);
    push(pubkey);
}
size_t Witness::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        cur_element = 0;
        curLen = 0;
        cur_element_len = 0;
        cur_bytes_parsed = 0;
        reset();
        clear();
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    // reading elements len varint
    if(s->available() && bytes_parsed == 0){ 
        lenLen = s->read();
        bytes_read++;
        if(lenLen < 0xfd){
            numElements = lenLen;
            lenLen = 1;
        }else{
            numElements = 0;
            status = PARSING_FAILED;
            bytes_parsed += bytes_read;
            return bytes_read;
        }
    }
    if(bytes_parsed+bytes_read == lenLen && lenVarInt(numElements) != lenLen){
        status = PARSING_FAILED;
        bytes_parsed+=bytes_read;
        return bytes_read;
    }
    // reading elements
    while(s->available() && cur_element<numElements){
        // beginning of the element
        size_t offset = bytes_parsed+bytes_read-lenVarInt(numElements)-cur_bytes_parsed;
        size_t cur_bytes_read = 0;
        // reading scriptLen varint
        if(s->available() && cur_bytes_parsed == 0){ 
            curLen = s->read();
            cur_bytes_read++;
            if(curLen < 0xfd){
                cur_element_len = curLen;
                curLen = 1;
            }else{
                cur_element_len = 0;
                curLen = 1+(1 << (curLen - 0xfc));
            }
        }
        while(s->available() && cur_bytes_parsed+cur_bytes_read < curLen){
            cur_element_len += (s->read() << (8*(cur_bytes_parsed+cur_bytes_read-1)));
            cur_bytes_read++;
        }
        if(lenVarInt(cur_element_len) != curLen && cur_bytes_parsed+cur_bytes_read == curLen){
            status = PARSING_FAILED;
            cur_bytes_parsed+=cur_bytes_read;
            bytes_read+=cur_bytes_read;
            bytes_parsed+=bytes_read;
            return bytes_read;
        }
        if(cur_bytes_parsed+cur_bytes_read == curLen && cur_bytes_read>0){
            if(witnessLen==0){
                witnessLen = cur_element_len+lenVarInt(cur_element_len);
                witnessArray = (uint8_t *)calloc(witnessLen, sizeof(uint8_t));
                writeVarInt(cur_element_len, witnessArray, lenVarInt(cur_element_len));
            }else{
                witnessArray = (uint8_t *)realloc( witnessArray, (witnessLen + cur_element_len + lenVarInt(cur_element_len)) * sizeof(uint8_t));
                witnessLen += cur_element_len+lenVarInt(cur_element_len);
                writeVarInt(cur_element_len, witnessArray+offset, lenVarInt(cur_element_len));
            }
        }
        while(s->available() > 0 && cur_bytes_parsed+cur_bytes_read < cur_element_len+lenVarInt(cur_element_len)){
            witnessArray[offset+cur_bytes_parsed+cur_bytes_read] = s->read();
            // s->read();
            cur_bytes_read++;
        }
        if(cur_bytes_parsed+cur_bytes_read==cur_element_len+lenVarInt(cur_element_len)){
            curLen = 0;
            cur_element_len = 0;
            cur_element++;
            cur_bytes_parsed=0;
        }else{
            cur_bytes_parsed += cur_bytes_read;
        }
        bytes_read += cur_bytes_read;
    }
    if(cur_element==numElements){
        status = PARSING_DONE;
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}
size_t Witness::to_stream(SerializeStream *s, size_t offset) const{
    size_t bytes_written = 0;
    // varint
    uint8_t l = lenVarInt(numElements);
    uint8_t arr[10];
    writeVarInt(numElements, arr, sizeof(arr));
    while(s->available() && bytes_written+offset < l){
        s->write(arr[offset+bytes_written]);
        bytes_written++;
    }
    while(s->available() && bytes_written+offset < l+witnessLen){
        s->write(witnessArray[bytes_written+offset-l]);
        bytes_written++;
    }
    return bytes_written;
}
size_t Witness::length() const{
    return witnessLen + lenVarInt(numElements);
}
size_t Witness::push(const uint8_t * data, size_t len){
    if(witnessLen + len + lenVarInt(len) > MAX_SCRIPT_SIZE){
        clear();
        return 0;
    }
    if(witnessLen == 0){
        witnessArray = (uint8_t *) calloc( len + lenVarInt(len), sizeof(uint8_t));
    }else{
        witnessArray = (uint8_t *) realloc( witnessArray, (witnessLen + len + lenVarInt(len)) * sizeof(uint8_t));
    }
    writeVarInt(len, witnessArray+witnessLen, lenVarInt(len));
    memcpy(witnessArray + witnessLen + lenVarInt(len), data, len);
    witnessLen += len+lenVarInt(len);
    numElements++;
    return witnessLen;
}
size_t Witness::push(const PublicKey pubkey){
    uint8_t sec[65];
    uint8_t len = pubkey.sec(sec, sizeof(sec));
    push(sec, len);
    return witnessLen;
}
size_t Witness::push(const Signature sig, SigHashType sigType){
    uint8_t der[75];
    uint8_t len = sig.der(der, sizeof(der));
    der[len] = sigType;
    push(der, len+1);
    return witnessLen;
}
size_t Witness::push(const Script sc){
    uint8_t len = sc.length();
    uint8_t * tmp;
    tmp = (uint8_t *)calloc(len, sizeof(uint8_t));
    size_t l = sc.serialize(tmp, len);
    size_t dl = readVarInt(tmp, len);
    push(tmp+l-dl, dl);
    free(tmp);
    return witnessLen;
}
Witness::Witness(const Witness &other){
    init();
    numElements = other.numElements;
    if(other.witnessLen > 0){
        witnessLen = other.witnessLen;
        witnessArray = (uint8_t *) calloc( witnessLen, sizeof(uint8_t));
        memcpy(witnessArray, other.witnessArray, witnessLen);
    }
};
Witness &Witness::operator=(Witness const &other){
    clear();
    numElements = other.numElements;
    if(other.witnessLen > 0){
        witnessLen = other.witnessLen;
        witnessArray = (uint8_t *) calloc( witnessLen, sizeof(uint8_t));
        memcpy(witnessArray, other.witnessArray, witnessLen);
    }
    return *this;
};