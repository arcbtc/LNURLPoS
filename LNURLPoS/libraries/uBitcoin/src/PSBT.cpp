#include "PSBT.h"
#include "Conversion.h"
#if USE_STD_STRING
using std::string;
#define String string
#endif

#define UBTC_ERR_PSBT_MAGIC 1
#define UBTC_ERR_PSBT_SCOPE 2
#define UBTC_ERR_PSBT_KEY 	3
#define UBTC_ERR_PSBT_VALUE 4
#define UBTC_ERR_PSBT_TX    5
#define UBTC_ERR_PSBT_IN    6
#define UBTC_ERR_PSBT_OUT   7

// descriptor checksum from https://github.com/bitcoin/bitcoin/blob/master/src/script/descriptor.cpp
uint64_t PolyMod(uint64_t c, int val){
    uint8_t c0 = c >> 35;
    c = ((c & 0x7ffffffff) << 5) ^ val;
    if (c0 & 1) c ^= 0xf5dee51989;
    if (c0 & 2) c ^= 0xa9fdca3312;
    if (c0 & 4) c ^= 0x1bab10e32d;
    if (c0 & 8) c ^= 0x3706b1677a;
    if (c0 & 16) c ^= 0x644d626ffd;
    return c;
}

size_t descriptorChecksum(const char * span, size_t spanLen, char * output, size_t outputSize){
    /** A character set designed such that:
     *  - The most common 'unprotected' descriptor characters (hex, keypaths) are in the first group of 32.
     *  - Case errors cause an offset that's a multiple of 32.
     *  - As many alphabetic characters are in the same group (while following the above restrictions).
     *
     * If p(x) gives the position of a character c in this character set, every group of 3 characters
     * (a,b,c) is encoded as the 4 symbols (p(a) & 31, p(b) & 31, p(c) & 31, (p(a) / 32) + 3 * (p(b) / 32) + 9 * (p(c) / 32).
     * This means that changes that only affect the lower 5 bits of the position, or only the higher 2 bits, will just
     * affect a single symbol.
     *
     * As a result, within-group-of-32 errors count as 1 symbol, as do cross-group errors that don't affect
     * the position within the groups.
     */
    memset(output, 0, outputSize);
    if(outputSize < 8){
        return 0;
    }
    static const char * INPUT_CHARSET = "0123456789()[],'/*abcdefgh@:$%{}IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";

    /** The character set for the checksum itself (same as bech32). */
    static const char * CHECKSUM_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    uint64_t c = 1;
    int cls = 0;
    int clscount = 0;
    // size_t len = strlen(span);
    for(size_t i=0; i<spanLen; i++){
        char ch = span[i];
        const char * pch = strchr(INPUT_CHARSET, ch);
        if(pch==NULL){ // char not in the alphabet
            return 0;
        }
        size_t pos = pch - INPUT_CHARSET;
        c = PolyMod(c, pos & 31); // Emit a symbol for the position inside the group, for every character.
        cls = cls * 3 + (pos >> 5); // Accumulate the group numbers
        if (++clscount == 3) {
            // Emit an extra symbol representing the group numbers, for every 3 characters.
            c = PolyMod(c, cls);
            cls = 0;
            clscount = 0;
        }
    }
    if (clscount > 0) c = PolyMod(c, cls);
    for (int j = 0; j < 8; ++j) c = PolyMod(c, 0); // Shift further to determine the checksum.
    c ^= 1; // Prevent appending zeroes from not affecting the checksum.

    memset(output, ' ', 8);
    for (int j = 0; j < 8; ++j) output[j] = CHECKSUM_CHARSET[(c >> (5 * (7 - j))) & 31];
    return 8;
}

#if (USE_ARDUINO_STRING || USE_STD_STRING)
String descriptorChecksum(String descriptor){
    char checksum[10] = { 0 };
    descriptorChecksum(descriptor.c_str(), strlen(descriptor.c_str()), checksum, sizeof(checksum));
    return String(checksum);
}
#endif

size_t PSBT::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        // free memory
        if(tx.inputsNumber > 0){
            for(size_t i=0; i<tx.inputsNumber; i++){
                if(txInsMeta[i].derivationsLen > 0){
                    for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
                        if(txInsMeta[i].derivations[j].derivationLen > 0){
                            free(txInsMeta[i].derivations[j].derivation);
                        }
                    }
                    delete [] txInsMeta[i].derivations;
                }
                if(txInsMeta[i].signaturesLen > 0){
                    delete [] txInsMeta[i].signatures;
                }
            }
            delete [] txInsMeta;
        }
        if(tx.outputsNumber > 0){
            for(size_t i=0; i<tx.outputsNumber; i++){
                if(txOutsMeta[i].derivationsLen > 0){
                    for(size_t j=0; j<txOutsMeta[i].derivationsLen; j++){
                        if(txOutsMeta[i].derivations[j].derivationLen > 0){
                            free(txOutsMeta[i].derivations[j].derivation);
                        }
                    }
                    delete [] txOutsMeta[i].derivations;
                }
            }
            delete [] txOutsMeta;
        }
        tx.reset();
        bytes_parsed = 0;
        current_section = 0;
        last_key_pos = 5;
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    uint8_t prefix[] = {0x70, 0x73, 0x62, 0x74, 0xFF};
    while(s->available() && bytes_read+bytes_parsed < 5){
        uint8_t c = s->read();
        bytes_read++;
        if(c != prefix[bytes_read+bytes_parsed-1]){
            status = PARSING_FAILED;
            ubtc_errno = UBTC_ERR_PSBT_MAGIC;
            bytes_parsed += bytes_read;
            return bytes_read;
        }
    }
    // global scope
    if(bytes_read+bytes_parsed == last_key_pos){
        bytes_read += s->parse(&key);
        bytes_read += s->parse(&value);
    }
    while(s->available() && key.getStatus() == PARSING_INCOMPLETE){
        bytes_read += s->parse(&key);
    }
    if(key.getStatus() == PARSING_FAILED){
        status = PARSING_FAILED;
        ubtc_errno = UBTC_ERR_PSBT_KEY;
        bytes_parsed += bytes_read;
        return bytes_read;
    }
    while(s->available() && value.getStatus() == PARSING_INCOMPLETE){
        bytes_read += s->parse(&value);
    }
    if(value.getStatus() == PARSING_FAILED){
        status = PARSING_FAILED;
        ubtc_errno = UBTC_ERR_PSBT_VALUE;
        bytes_parsed += bytes_read;
        return bytes_read;
    }
    if(last_key_pos == 5 && value.getStatus() == PARSING_DONE && key.getStatus() == PARSING_DONE){
        uint8_t * arr = (uint8_t *)calloc(key.length(), sizeof(uint8_t));
        if(arr == NULL){ status = PARSING_FAILED; return 0; }
        key.serialize(arr, key.length());
        if(key.length() != 2 || arr[0] != 1 || arr[1] != 0){
            status = PARSING_FAILED;
            ubtc_errno = UBTC_ERR_PSBT_SCOPE;
        }
        free(arr);
        if(status == PARSING_FAILED){
            bytes_parsed += bytes_read;
            return bytes_read;
        }
        arr = (uint8_t *)calloc(value.length(), sizeof(uint8_t));
        if(arr == NULL){ status = PARSING_FAILED; return 0; }
        value.serialize(arr, value.length());
        size_t l = lenVarInt(value.length());
        tx.parse(arr+l, value.length()-l);
        if(tx.getStatus() != PARSING_DONE){
            status = PARSING_FAILED;
        }
        free(arr);
        if(status == PARSING_FAILED){
            bytes_parsed += bytes_read;
            return bytes_read;
        }
        txInsMeta = new PSBTInputMetadata[tx.inputsNumber];
        for(size_t i=0; i<tx.inputsNumber; i++){
            txInsMeta[i].derivationsLen = 0;
            txInsMeta[i].signaturesLen = 0;
        }
        txOutsMeta = new PSBTOutputMetadata[tx.outputsNumber];
        for(size_t i=0; i<tx.outputsNumber; i++){
            txOutsMeta[i].derivationsLen = 0;
        }
        last_key_pos += key.length()+value.length();
    }
    uint8_t sections_number = 0;
    if(last_key_pos > 5){ // tx is already parsed
        sections_number = 1+tx.inputsNumber+tx.outputsNumber;
    }
    // parsing keys and values
    while(s->available() && current_section < sections_number){
        if(key.getStatus() == PARSING_DONE && value.getStatus() == PARSING_DONE){
            bytes_read += s->parse(&key);
        }
        if(key.getStatus() == PARSING_DONE){
            if(key.length() == 1){ // delimiter
                current_section ++;
                last_key_pos += key.length();
                continue;
            }else{
                bytes_read += s->parse(&value);
            }
        }
        if(key.getStatus() == PARSING_FAILED || value.getStatus() == PARSING_FAILED){
            status = PARSING_FAILED;
            ubtc_errno = (key.getStatus() == PARSING_FAILED) ? UBTC_ERR_PSBT_KEY : UBTC_ERR_PSBT_VALUE;
            bytes_parsed += bytes_read;
            return bytes_read;
        }
        if(key.getStatus() == PARSING_INCOMPLETE){
            bytes_read += s->parse(&key);
        }
        if(value.getStatus() == PARSING_INCOMPLETE){
            bytes_read += s->parse(&value);
        }
        if(key.getStatus() == PARSING_DONE && value.getStatus() == PARSING_DONE){
            int res = add(current_section, &key, &value);
            if(res < 0){
                status = PARSING_FAILED;
                ubtc_errno = UBTC_ERR_PSBT_SCOPE;
                bytes_parsed += bytes_read;
                return bytes_read;
            }
            last_key_pos += key.length() + value.length();
        }
    }
    if(current_section == sections_number && sections_number > 0){
        status = PARSING_DONE;
        key = Script();
        value = Script();
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}

int PSBT::add(uint8_t section, const Script * k, const Script * v){
    if(section == 0 || section > 1+tx.inputsNumber+tx.outputsNumber){
        return 0;
    }
    uint8_t * key_arr = (uint8_t *)calloc(k->length(), sizeof(uint8_t));
    if(key_arr == NULL){ return 0; }
    k->serialize(key_arr, k->length());
    uint8_t * val_arr = (uint8_t *)calloc(v->length(), sizeof(uint8_t));
    if(val_arr == NULL){ free(key_arr); return 0; }
    v->serialize(val_arr, v->length());
    uint8_t key_code = key_arr[lenVarInt(k->length())];
    int res = 0;

    if(section < 1+tx.inputsNumber){ // input section
        uint8_t input = section-1;
        switch(key_code){
            case 0: { // PSBT_IN_NON_WITNESS_UTXO
                // we need to verify that tx hashes to prevtx_hash
                // and get corresponding txOut from it. We don't need to keep the tx itself.
                if(k->length() != 2){
                    res = -1;
                    break;
                }
                Tx tempTx;
                tempTx.parse(val_arr+lenVarInt(v->length()), v->length()-lenVarInt(v->length()));
                if(tempTx.getStatus() != PARSING_DONE){
                    res = -2;
                    break;
                }
                uint8_t hash[32];
                tempTx.hash(hash);
                if(memcmp(hash, tx.txIns[input].hash, 32) != 0){
                    res = -3;
                    break;
                }
                if(tempTx.outputsNumber <= tx.txIns[input].outputIndex){
                    res = -3;
                    break;
                }
                txInsMeta[input].txOut = tempTx.txOuts[tx.txIns[input].outputIndex];
                res = 1;
                break;
            }
            case 1: { // PSBT_IN_WITNESS_UTXO
                if(k->length() != 2){
                    res = -1;
                    break;
                }
                txInsMeta[input].txOut.parse(val_arr+lenVarInt(v->length()), v->length()-lenVarInt(v->length()));
                if(txInsMeta[input].txOut.getStatus() != PARSING_DONE){
                    res = -2;
                    break;
                }
                res = 1;
                break;
            }
            case 2: { // PSBT_IN_PARTIAL_SIG
                if(k->length() != 35 && k->length() != 67){
                    res = -1;
                    break;
                }
                PSBTPartialSignature psig;
                psig.pubkey.parse(key_arr+2, k->length()-2);
                if(psig.pubkey.getStatus() != PARSING_DONE){
                    res = -1;
                    break;
                }
                psig.signature.parse(val_arr+1, v->length()-1);
                if(psig.signature.getStatus() != PARSING_DONE){
                    res = -2;
                    break;
                }
                if(txInsMeta[input].signaturesLen == 0){
                    txInsMeta[input].signaturesLen = 1;
                    txInsMeta[input].signatures = new PSBTPartialSignature[txInsMeta[input].signaturesLen];
                }else{
                    PSBTPartialSignature * p = txInsMeta[input].signatures;
                    txInsMeta[input].signatures = new PSBTPartialSignature[txInsMeta[input].signaturesLen+1];
                    for(size_t i=0; i<txInsMeta[input].signaturesLen; i++){
                        txInsMeta[input].signatures[i] = p[i];
                    }
                    txInsMeta[input].signaturesLen++;
                    delete [] p;
                }
                txInsMeta[input].signatures[txInsMeta[input].signaturesLen-1] = psig;
                res = 1;
                break;
            }
            case 3: { // PSBT_IN_SIGHASH_TYPE
                // not implemented
                break;
            }
            case 4: { // PSBT_IN_REDEEM_SCRIPT
                if(k->length() != 2){
                    res = -1;
                    break;
                }
                txInsMeta[input].redeemScript.parse(val_arr, v->length());
                res = 1;
                break;
            }
            case 5: { // PSBT_IN_WITNESS_SCRIPT
                if(k->length() != 2){
                    res = -1;
                    break;
                }
                txInsMeta[input].witnessScript.parse(val_arr, v->length());
                res = 1;
                break;
            }
            case 6: { // PSBT_IN_BIP32_DERIVATION
                // TODO: move to function
                if(k->length() != 35 && k->length() != 67){
                    res = -1;
                    break;
                }
                if(txInsMeta[input].derivationsLen == 0){
                    txInsMeta[input].derivationsLen = 1;
                    txInsMeta[input].derivations = new PSBTDerivation[txInsMeta[input].derivationsLen];
                }else{
                    PSBTDerivation * p = txInsMeta[input].derivations;
                    txInsMeta[input].derivations = new PSBTDerivation[txInsMeta[input].derivationsLen+1];
                    for(size_t i=0; i<txInsMeta[input].derivationsLen; i++){
                        txInsMeta[input].derivations[i] = p[i];
                    }
                    txInsMeta[input].derivationsLen++;
                    delete [] p;
                }
                PSBTDerivation * der = &txInsMeta[input].derivations[txInsMeta[input].derivationsLen-1];
                der->pubkey.parse(key_arr+2, k->length()-2);
                if(der->pubkey.getStatus() != PARSING_DONE){
                    res = -1;
                    break;
                }
                memcpy(der->fingerprint, val_arr+lenVarInt(v->length()), 4);
                der->derivationLen = (v->length()-lenVarInt(v->length())-4)/sizeof(uint32_t);
                der->derivation = (uint32_t *)calloc(der->derivationLen, sizeof(uint32_t));
                if(der->derivation == NULL){ der->derivationLen = 0; res = -1; break; }
                for(size_t i=0; i<der->derivationLen; i++){
                    der->derivation[i] = littleEndianToInt(val_arr+lenVarInt(v->length())+4*(i+1),4);
                }
                res = 1;
                break;
            }
            case 7: { // PSBT_IN_FINAL_SCRIPTSIG
                // not implemented
                break;
            }
            case 8: { // PSBT_IN_FINAL_SCRIPTWITNESS
                // not implemented
                break;
            }
        }
    }else{ // output section
        uint8_t output = section-1-tx.inputsNumber;
        switch(key_code){
            case 0: { // PSBT_OUT_REDEEM_SCRIPT
                if(k->length() != 2){
                    return -1;
                }
                txOutsMeta[output].redeemScript.parse(val_arr, v->length());
                res = 1;
                break;
            }
            case 1: { // PSBT_OUT_WITNESS_SCRIPT
                if(k->length() != 2){
                    return -1;
                }
                txOutsMeta[output].witnessScript.parse(val_arr, v->length());
                res = 1;
                break;
            }
            case 2: { // PSBT_OUT_BIP32_DERIVATION
                // TODO: move to function
                if(k->length() != 35 && k->length() != 67){
                    res = -1;
                    break;
                }
                if(txOutsMeta[output].derivationsLen == 0){
                    txOutsMeta[output].derivationsLen = 1;
                    txOutsMeta[output].derivations = new PSBTDerivation[txOutsMeta[output].derivationsLen];
                }else{
                    PSBTDerivation * p = txOutsMeta[output].derivations;
                    txOutsMeta[output].derivations = new PSBTDerivation[txOutsMeta[output].derivationsLen+1];
                    for(int i=0; i<txOutsMeta[output].derivationsLen; i++){
                        txOutsMeta[output].derivations[i] = p[i];
                    }
                    txOutsMeta[output].derivationsLen++;
                    delete [] p;
                }
                PSBTDerivation * der = &txOutsMeta[output].derivations[txOutsMeta[output].derivationsLen-1];
                der->pubkey.parse(key_arr+2, k->length()-2);
                if(der->pubkey.getStatus() != PARSING_DONE){
                    res = -1;
                    break;
                }
                memcpy(der->fingerprint, val_arr+lenVarInt(v->length()), 4);
                der->derivationLen = (v->length()-lenVarInt(v->length())-4)/sizeof(uint32_t);
                der->derivation = (uint32_t *)calloc(der->derivationLen, sizeof(uint32_t));
                if(der->derivation == NULL){ der->derivationLen = 0; res = -1; break; }
                for(size_t i=0; i<der->derivationLen; i++){
                    der->derivation[i] = littleEndianToInt(val_arr+lenVarInt(v->length())+4*(i+1),4);
                }
                res = 1;
                break;
            }
        }
    }
    free(key_arr);
    free(val_arr);
    return res; // by default - ignore the key-value pair
}

size_t PSBT::to_stream(SerializeStream *s, size_t offset) const{
    // PSBT prefix + raw transaction key
    uint8_t prefix[] = {0x70, 0x73, 0x62, 0x74, 0xff, 0x01, 0x00};
    size_t bytes_written = 0;
    while(s->available() && bytes_written+offset < 7){
        s->write(prefix[bytes_written+offset]);
        bytes_written++;
    }
    size_t cur = 7;
    uint8_t arr[10];
    size_t l = writeVarInt(tx.length(), arr, 10);
    while(s->available() && bytes_written+offset < cur+l){
        s->write(arr[bytes_written+offset-cur]);
        bytes_written++;
    }
    cur+=l;
    while(s->available() && bytes_written+offset < cur+tx.length()){
        bytes_written += s->serialize(&tx, offset+bytes_written-cur);
    }
    cur+=tx.length();
    uint8_t sections_number = 1 + tx.inputsNumber + tx.outputsNumber;
    uint8_t section = 0;
    while(s->available() && section < sections_number){
        if(section > 0 && section < tx.inputsNumber+1){
            uint8_t input = section-1;
            for(size_t i=0; i<txInsMeta[input].signaturesLen; i++){
                uint8_t key_arr[67];
                key_arr[1] = 0x02; // PSBT_IN_PARTIAL_SIG
                uint8_t key_len = 1+txInsMeta[input].signatures[i].pubkey.serialize(key_arr+2, 65);
                key_arr[0] = key_len;
                while(s->available() && bytes_written+offset-cur < (size_t)key_len+1){
                    s->write(key_arr[bytes_written+offset-cur]);
                    bytes_written++;
                }
                cur += key_len+1;
                uint8_t val_arr[100];
                uint8_t val_len = 1+txInsMeta[input].signatures[i].signature.serialize(val_arr+1, 98);
                val_arr[0] = val_len;
                val_arr[val_len] = SIGHASH_ALL;
                while(s->available() && bytes_written+offset-cur < (size_t)val_len+1){
                    s->write(val_arr[bytes_written+offset-cur]);
                    bytes_written++;
                }
                cur += val_len+1;
            }
        }
        s->write(0);
        bytes_written++;
        section++;
        cur++;
    }
    return bytes_written;
}

size_t PSBT::length() const{
    uint8_t sections_number = 1 + tx.inputsNumber + tx.outputsNumber;
    size_t len = 7 + lenVarInt(tx.length()) + tx.length() + sections_number;
    for(size_t input=0; input<tx.inputsNumber; input++){
        for(size_t i=0; i<txInsMeta[input].signaturesLen; i++){
            len += 2+txInsMeta[input].signatures[i].pubkey.length();
            len += 2+txInsMeta[input].signatures[i].signature.length();
        }
    }
    return len;
}

PSBT::PSBT(PSBT const &other):PSBT(){
    tx = other.tx;
    status = other.status;
    txInsMeta = new PSBTInputMetadata[tx.inputsNumber];
    txOutsMeta = new PSBTOutputMetadata[tx.outputsNumber];
    for(size_t i=0; i<tx.inputsNumber; i++){
        txInsMeta[i] = other.txInsMeta[i];
        txInsMeta[i].derivations = new PSBTDerivation[txInsMeta[i].derivationsLen];
        for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
            txInsMeta[i].derivations[j] = other.txInsMeta[i].derivations[j];
            txInsMeta[i].derivations[j].derivation = (uint32_t *)calloc(txInsMeta[i].derivations[j].derivationLen, sizeof(uint32_t));
            if(txInsMeta[i].derivations[j].derivation == NULL){
                txInsMeta[i].derivations[j].derivationLen = 0;
            }else{
                memcpy(txInsMeta[i].derivations[j].derivation, other.txInsMeta[i].derivations[j].derivation, txInsMeta[i].derivations[j].derivationLen*sizeof(uint32_t));
            }
        }
        txInsMeta[i].signatures = new PSBTPartialSignature[txInsMeta[i].signaturesLen];
        for(size_t j=0; j<txInsMeta[i].signaturesLen; j++){
            txInsMeta[i].signatures[j] = other.txInsMeta[i].signatures[j];
        }
    }
    for(size_t i=0; i<tx.outputsNumber; i++){
        txOutsMeta[i] = other.txOutsMeta[i];
        txOutsMeta[i].derivations = new PSBTDerivation[txOutsMeta[i].derivationsLen];
        for(size_t j=0; j<txOutsMeta[i].derivationsLen; j++){
            txOutsMeta[i].derivations[j] = other.txOutsMeta[i].derivations[j];
            txOutsMeta[i].derivations[j].derivation = (uint32_t *)calloc(txOutsMeta[i].derivations[j].derivationLen, sizeof(uint32_t));
            if(txOutsMeta[i].derivations[j].derivation == NULL){
                txOutsMeta[i].derivations[j].derivationLen = 0;
            }else{
                memcpy(txOutsMeta[i].derivations[j].derivation, other.txOutsMeta[i].derivations[j].derivation, txOutsMeta[i].derivations[j].derivationLen*sizeof(uint32_t));
            }
        }
    }
}

PSBT::~PSBT(){
    // free memory
    if(tx.inputsNumber > 0){
        for(size_t i=0; i<tx.inputsNumber; i++){
            if(txInsMeta[i].derivationsLen > 0){
                for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
                    if(txInsMeta[i].derivations[j].derivationLen > 0){
                        free(txInsMeta[i].derivations[j].derivation);
                    }
                }
                delete [] txInsMeta[i].derivations;
            }
            if(txInsMeta[i].signaturesLen > 0){
                delete [] txInsMeta[i].signatures;
            }
        }
        delete [] txInsMeta;
    }
    if(tx.outputsNumber > 0){
        for(size_t i=0; i<tx.outputsNumber; i++){
            if(txOutsMeta[i].derivationsLen > 0){
                for(size_t j=0; j<txOutsMeta[i].derivationsLen; j++){
                    if(txOutsMeta[i].derivations[j].derivationLen > 0){
                        free(txOutsMeta[i].derivations[j].derivation);
                    }
                }
                delete [] txOutsMeta[i].derivations;
            }
        }
        delete [] txOutsMeta;
    }
}

uint8_t PSBT::sign(const HDPrivateKey root){
    uint8_t fingerprint[4];
    root.fingerprint(fingerprint);
    uint8_t counter = 0;
    // in most cases only one account key is required, so we can cache it
    uint32_t * first_derivation = NULL;
    uint8_t first_derivation_len = 0;
    HDPrivateKey account;
    for(size_t i=0; i<tx.inputsNumber; i++){
        if(txInsMeta[i].derivationsLen > 0){
            for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
                if(memcmp(fingerprint, txInsMeta[i].derivations[j].fingerprint, 4) == 0){
                    // caching account key here
                    if(first_derivation == NULL){
                        first_derivation = txInsMeta[i].derivations[j].derivation;
                        first_derivation_len = 0;
                        for(size_t k=0; k < txInsMeta[i].derivations[j].derivationLen; k++){
                            if(txInsMeta[i].derivations[j].derivation[k] >= 0x80000000){
                                first_derivation_len++;
                            }else{
                                break;
                            }
                        }
                        account = root.derive(first_derivation, first_derivation_len);
                    }
                    PrivateKey pk;
                    // checking if cached key is ok
                    if(memcmp(first_derivation, txInsMeta[i].derivations[j].derivation, first_derivation_len*sizeof(uint32_t))==0){
                        pk = account.derive(txInsMeta[i].derivations[j].derivation+first_derivation_len, txInsMeta[i].derivations[j].derivationLen - first_derivation_len);
                    }else{
                        pk = root.derive(txInsMeta[i].derivations[j].derivation, txInsMeta[i].derivations[j].derivationLen);
                    }
                    if(txInsMeta[i].derivations[j].pubkey == pk.publicKey()){
                        // can sign - let's sign
                        uint8_t h[32];
                        if(txInsMeta[i].witnessScript.length() > 1){ // P2WSH / P2SH_P2WSH
                            tx.sigHashSegwit(h, i, txInsMeta[i].witnessScript, txInsMeta[i].txOut.amount);
                        }else{
                            if(txInsMeta[i].redeemScript.length() > 1){
                                if(txInsMeta[i].redeemScript.type() == P2WPKH){ // P2SH_P2WPKH
                                    // tx.sigHashSegwit(h, i, txInsMeta[i].redeemScript, txInsMeta[i].txOut.amount);
                                    tx.sigHashSegwit(h, i, pk.publicKey().script(), txInsMeta[i].txOut.amount);
                                }else{ // P2SH
                                    tx.sigHash(h, i, txInsMeta[i].redeemScript);
                                }
                            }else{ // P2WPKH / P2PKH / DIRECT_SCRIPT
                                if(txInsMeta[i].txOut.scriptPubkey.type() == P2WPKH){
                                    // tx.sigHashSegwit(h, i, txInsMeta[i].txOut.scriptPubkey, txInsMeta[i].txOut.amount);
                                    tx.sigHashSegwit(h, i, pk.publicKey().script(), txInsMeta[i].txOut.amount);
                                }else{ // P2PKH / DIRECT_SCRIPT
                                    tx.sigHash(h, i, txInsMeta[i].txOut.scriptPubkey);
                                }
                            }
                        }
                        Signature sig = pk.sign(h);

                        // adding partial signature to the PSBT
                        uint8_t arr[67];
                        arr[1] = 0x02; // PSBT_IN_PARTIAL_SIG
                        uint8_t len = 1 + pk.publicKey().serialize(arr+2, 65);
                        arr[0] = len;
                        Script k;
                        k.parse(arr, len+1);

                        uint8_t varr[100];
                        len = 1+sig.serialize(varr+1, 99);
                        varr[0] = len;
                        varr[len] = SIGHASH_ALL;
                        Script val;
                        val.parse(varr, len+1);
                        add(i+1, &k, &val);
                        counter++; // can sign
                    }
                }
            }
        }
    }
    return counter;
}

// TODO: refactor, super unefficient
#if USE_ARDUINO_STRING || USE_STD_STRING
size_t PSBT::parseBase64(String b64){
    String s = base64ToHex(b64);
    parse(s);
    return s.length()/2;
}
String PSBT::toBase64(){
    return hexToBase64(toString());
}
#endif

uint64_t PSBT::fee() const{
    uint64_t input_amount = 0;
    uint64_t output_amount = 0;
    for(size_t i=0; i<tx.inputsNumber; i++){
        if(txInsMeta[i].txOut.amount == 0){
            return 0;
        }
        input_amount += txInsMeta[i].txOut.amount;
    }
    for(size_t i=0; i<tx.outputsNumber; i++){
        output_amount += tx.txOuts[i].amount;
    }
    if(output_amount > input_amount){
        return 0;
    }
    return input_amount-output_amount;
}

bool PSBT::isMine(uint8_t outputNumber, const HDPublicKey xpub) const{
    bool mine = false;
    if(txOutsMeta[outputNumber].derivationsLen > 0){
        for(unsigned int j=0; j<txOutsMeta[outputNumber].derivationsLen; j++){
            HDPublicKey pub = xpub.derive(txOutsMeta[outputNumber].derivations[j].derivation, txOutsMeta[outputNumber].derivations[j].derivationLen);
            if(memcmp(pub.point, txOutsMeta[outputNumber].derivations[j].pubkey.point, 64)==0){
                mine = true;
            }
        }
    }
    // TODO: add verification of the script
    return mine;
}

bool PSBT::isMine(uint8_t outputNumber, const HDPrivateKey xprv) const{
    bool mine = false;
    if(txOutsMeta[outputNumber].derivationsLen > 0){
        for(unsigned int j=0; j<txOutsMeta[outputNumber].derivationsLen; j++){
            HDPublicKey pub = xprv.derive(txOutsMeta[outputNumber].derivations[j].derivation, txOutsMeta[outputNumber].derivations[j].derivationLen).xpub();
            if(memcmp(pub.point, txOutsMeta[outputNumber].derivations[j].pubkey.point, 64)==0){
                mine = true;
            }
        }
    }
    // TODO: add verification of the script
    return mine;
}

PSBT& PSBT::operator=(PSBT const &other){
    if (this == &other){ return *this; } // self-assignment
    // free memory
    if(tx.inputsNumber > 0){
        for(size_t i=0; i<tx.inputsNumber; i++){
            if(txInsMeta[i].derivationsLen > 0){
                for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
                    if(txInsMeta[i].derivations[j].derivationLen > 0){
                        free(txInsMeta[i].derivations[j].derivation);
                    }
                }
                delete [] txInsMeta[i].derivations;
            }
            if(txInsMeta[i].signaturesLen > 0){
                delete [] txInsMeta[i].signatures;
            }
        }
        delete [] txInsMeta;
    }
    if(tx.outputsNumber > 0){
        for(size_t i=0; i<tx.outputsNumber; i++){
            if(txOutsMeta[i].derivationsLen > 0){
                for(size_t j=0; j<txOutsMeta[i].derivationsLen; j++){
                    if(txOutsMeta[i].derivations[j].derivationLen > 0){
                        free(txOutsMeta[i].derivations[j].derivation);
                    }
                }
                delete [] txOutsMeta[i].derivations;
            }
        }
        delete [] txOutsMeta;
    }
    // copy
    tx = other.tx;
    status = other.status;
    if(tx.inputsNumber > 0){
        txInsMeta = new PSBTInputMetadata[tx.inputsNumber];
        for(size_t i=0; i<tx.inputsNumber; i++){
            txInsMeta[i] = other.txInsMeta[i];
            if(txInsMeta[i].derivationsLen > 0){
                txInsMeta[i].derivations = new PSBTDerivation[txInsMeta[i].derivationsLen];
                for(size_t j=0; j<txInsMeta[i].derivationsLen; j++){
                    txInsMeta[i].derivations[j] = other.txInsMeta[i].derivations[j];
                    txInsMeta[i].derivations[j].derivation = (uint32_t*)calloc(txInsMeta[i].derivations[j].derivationLen, sizeof(uint32_t));
                    if(txInsMeta[i].derivations[j].derivation == NULL){
                        txInsMeta[i].derivations[j].derivationLen = 0;
                    }else{
                        memcpy(txInsMeta[i].derivations[j].derivation, other.txInsMeta[i].derivations[j].derivation, txInsMeta[i].derivations[j].derivationLen*sizeof(uint32_t));
                    }
                }
            }
            if(txInsMeta[i].signaturesLen > 0){
                txInsMeta[i].signatures = new PSBTPartialSignature[txInsMeta[i].signaturesLen];
                for(size_t j=0; j<txInsMeta[i].signaturesLen; j++){
                    txInsMeta[i].signatures[j] = other.txInsMeta[i].signatures[j];
                }
            }
        }
    }
    if(tx.outputsNumber > 0){
        txOutsMeta = new PSBTOutputMetadata[tx.outputsNumber];
        for(size_t i=0; i<tx.outputsNumber; i++){
            txOutsMeta[i] = other.txOutsMeta[i];
            txOutsMeta[i].derivations = new PSBTDerivation[txOutsMeta[i].derivationsLen];
            for(size_t j=0; j<txOutsMeta[i].derivationsLen; j++){
                txOutsMeta[i].derivations[j] = other.txOutsMeta[i].derivations[j];
                txOutsMeta[i].derivations[j].derivation = (uint32_t *)calloc(txOutsMeta[i].derivations[j].derivationLen, sizeof(uint32_t));
                if(txOutsMeta[i].derivations[j].derivation == NULL){
                    txOutsMeta[i].derivations[j].derivationLen = 0;
                }else{
                    memcpy(txOutsMeta[i].derivations[j].derivation, other.txOutsMeta[i].derivations[j].derivation, txOutsMeta[i].derivations[j].derivationLen*sizeof(uint32_t));
                }
            }
        }
    }
    return *this;
}
