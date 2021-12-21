#include <stdint.h>
#include <string.h>
#include "Bitcoin.h"
#include "Hash.h"
#include "Conversion.h"
#include "utility/trezor/sha2.h"

#if USE_STD_STRING
using std::string;
#define String string
#endif

#define UBTC_ERR_TX_GLOBAL 1
#define UBTC_ERR_TX_INPUT  2
#define UBTC_ERR_TX_OUTPUT 3
#define UBTC_ERR_TX_SCRIPT 4

//-------------------------------------------------------------------------------------- Transaction Input
void TxIn::init(){
    outputIndex = 0;
    sequence = 0;
    memset(hash, 0, 32);
    status = PARSING_DONE;
    bytes_parsed = 0;
}
TxIn::TxIn(void){
    init();
}
TxIn::TxIn(const uint8_t prev_id[32], uint32_t prev_index, uint32_t sequence_number){
    init();
    outputIndex = prev_index;
    sequence = sequence_number;
    for(int i=0; i<32; i++){
        hash[i] = prev_id[31-i];
    }
}
TxIn::TxIn(const uint8_t prev_id[32], uint32_t prev_index, const Script script, uint32_t sequence_number){
    outputIndex = prev_index;
    sequence = sequence_number;
    for(int i=0; i<32; i++){
        hash[i] = prev_id[31-i];
    }
    scriptSig = script;
}
TxIn::TxIn(const char * prev_id, uint32_t prev_index, uint32_t sequence_number){
    init();
    memset(hash, 0, 32);
    if(strlen(prev_id) < 64){
        return;
    }
    outputIndex = prev_index;
    sequence = sequence_number;
    uint8_t arr[32];
    fromHex(prev_id, 64, arr, 32);
    for(int i=0; i<32; i++){
        hash[i] = arr[31-i];
    }
}
TxIn::TxIn(const char * prev_id, uint32_t prev_index, const Script script, uint32_t sequence_number){
    outputIndex = prev_index;
    sequence = sequence_number;
    if(strlen(prev_id) < 64){
        return;
    }
    uint8_t tmp[32];
    fromHex(prev_id, 64, tmp, 32);
    for(int i=0; i<32; i++){
        hash[i] = tmp[31-i];
    }
    scriptSig = script;
}
size_t TxIn::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        outputIndex = 0;
        sequence = 0;
        scriptSig = Script();
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    while(s->available() && bytes_read+bytes_parsed<32){
        hash[bytes_read+bytes_parsed] = s->read();
        bytes_read++;
    }
    while(s->available() && bytes_read+bytes_parsed<32+4){
        uint8_t c = s->read();
        outputIndex += (c << (8*(bytes_read+bytes_parsed-32)));
        bytes_read++;
    }
    if(s->available() && bytes_read+bytes_parsed == 32+4){
        bytes_read += s->parse(&scriptSig);
    }
    if(s->available() && scriptSig.getStatus() == PARSING_INCOMPLETE){
        bytes_read += s->parse(&scriptSig);
    }
    if(scriptSig.getStatus() == PARSING_FAILED){
        status = PARSING_FAILED;
        ubtc_errno = UBTC_ERR_TX_SCRIPT | UBTC_ERR_TX_INPUT;
        bytes_parsed+=bytes_read;
        return bytes_read;
    }
    while(s->available() && bytes_read+bytes_parsed < 32+4+scriptSig.length()+4){
        uint8_t c = s->read();
        sequence += (c << (8*(bytes_read+bytes_parsed-scriptSig.length()-32-4)));
        bytes_read++;
    }
    if(scriptSig.getStatus() == PARSING_DONE && bytes_read+bytes_parsed == 32+4+scriptSig.length()+4){
        status = PARSING_DONE;
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}
size_t TxIn::to_stream(SerializeStream *s, size_t offset) const{
    size_t bytes_written = 0;
    while(s->available() && bytes_written+offset < 32){
        s->write(hash[bytes_written+offset]);
        bytes_written++;
    }
    uint8_t arr[4];
    intToLittleEndian(outputIndex, arr, 4);
    while(s->available() && bytes_written+offset < 32+4){
        s->write(arr[bytes_written+offset-32]);
        bytes_written++;
    }
    size_t len = scriptSig.length();
    if(s->available() && bytes_written+offset < 32+4+len){
        bytes_written+=s->serialize(&scriptSig, bytes_written+offset-32-4);
    }
    intToLittleEndian(sequence, arr, 4);
    while(s->available() && bytes_written+offset < 32+4+len+4){
        s->write(arr[bytes_written+offset-(32+4+len)]);
        bytes_written++;
    }
    return bytes_written;
}
size_t TxIn::length() const{
    return 32+4+scriptSig.length()+4;
}

//-------------------------------------------------------------------------------------- Transaction Output

size_t TxOut::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        amount = 0;
        scriptPubkey.clear(); scriptPubkey.reset();
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    while(s->available() && bytes_read+bytes_parsed<8){
        uint64_t c = s->read();
        amount += (c << (8*(bytes_read+bytes_parsed-32)));
        bytes_read++;
    }
    if(s->available() && bytes_read+bytes_parsed == 8){
        bytes_read += s->parse(&scriptPubkey);
    }
    if(s->available() && scriptPubkey.getStatus() == PARSING_INCOMPLETE){
        bytes_read += s->parse(&scriptPubkey);
    }
    if(scriptPubkey.getStatus() == PARSING_FAILED){
        status = PARSING_FAILED;
        ubtc_errno = UBTC_ERR_TX_SCRIPT | UBTC_ERR_TX_OUTPUT;
        bytes_parsed+=bytes_read;
        return bytes_read;
    }
    if(scriptPubkey.getStatus() == PARSING_DONE && bytes_read+bytes_parsed == 8+scriptPubkey.length()){
        status = PARSING_DONE;
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}

size_t TxOut::to_stream(SerializeStream *s, size_t offset) const{
    size_t bytes_written = 0;
    uint8_t arr[8] = { 0 };
    intToLittleEndian(amount, arr, 8);
    while(s->available() && bytes_written+offset < 8){
        s->write(arr[bytes_written+offset]);
        bytes_written++;
    }
    size_t len = scriptPubkey.length();
    if(s->available() && bytes_written+offset < 8+len){
        bytes_written += s->serialize(&scriptPubkey, bytes_written+offset-8);
    }
    return bytes_written;
}

//-------------------------------------------------------------------------------------- Transaction
void Tx::init(){
    version = 1;
    inputsNumber = 0;
    outputsNumber = 0;
    txIns = NULL;
    txOuts = NULL;
    locktime = 0;
    segwit_flag = 1;
    status = PARSING_DONE;
    bytes_parsed = 0;
}
Tx::Tx(){
    init();
}
Tx::Tx(const Tx & other){
    init();
    version = other.version;
    inputsNumber = other.inputsNumber;
    outputsNumber = other.outputsNumber;
    txIns = new TxIn[inputsNumber];
    txOuts = new TxOut[outputsNumber];
    for(unsigned int i=0;i<inputsNumber;i++){
        txIns[i] = other.txIns[i];
    }
    for(unsigned int i=0;i<outputsNumber;i++){
        txOuts[i] = other.txOuts[i];
    }
    locktime = other.locktime;
    segwit_flag = other.segwit_flag;
    status = other.status;
    bytes_parsed = other.bytes_parsed;
}
Tx& Tx::operator=(Tx const &other){ // copy-paste =(
    if (this == &other){ return *this; } // self-assignment
    version = other.version;
    if(inputsNumber > 0){
        delete [] txIns;
    }
    if(outputsNumber > 0){
        delete [] txOuts;
    }
    inputsNumber = other.inputsNumber;
    outputsNumber = other.outputsNumber;
    txIns = new TxIn[inputsNumber];
    txOuts = new TxOut[outputsNumber];
    for(unsigned int i=0;i<inputsNumber;i++){
        txIns[i] = other.txIns[i];
    }
    for(unsigned int i=0;i<outputsNumber;i++){
        txOuts[i] = other.txOuts[i];
    }
    locktime = other.locktime;
    segwit_flag = other.segwit_flag;
    status = other.status;
    bytes_parsed = other.bytes_parsed;
    return *this;
}
Tx::~Tx(){
    clear();
}
void Tx::clear(){
    if(inputsNumber > 0){
        inputsNumber = 0;
        delete [] txIns;
        txIns = NULL;
    }
    if(outputsNumber > 0){
        outputsNumber = 0;
        delete [] txOuts;
        txOuts = NULL;
    }
}
size_t Tx::length() const{
    bool is_segwit = isSegwit();
    size_t l = 4+4+lenVarInt(inputsNumber)+lenVarInt(outputsNumber)+2*is_segwit;
    for(unsigned int i=0; i<inputsNumber; i++){
        l += txIns[i].length();
        if(is_segwit){
            l += txIns[i].witness.length();
        }
    }
    for(unsigned int i=0; i<outputsNumber; i++){
        l += txOuts[i].length();
    }
    return l;
}
bool Tx::isSegwit() const{
    for(unsigned int i=0; i<inputsNumber; i++){
        if(txIns[i].isSegwit()){
            return true;
        }
    }
    return false;
}
size_t Tx::to_stream(SerializeStream *s, size_t offset) const{
    size_t bytes_written = 0;
    uint8_t arr[10] = { 0 }; // we will store varints and other numbers here
    intToLittleEndian(version, arr, 4);
    while(s->available() && bytes_written+offset < 4){
        s->write(arr[bytes_written+offset]);
        bytes_written++;
    }
    bool is_segwit = isSegwit();
    if(is_segwit && s->available() && bytes_written+offset == 4){
        s->write(0x00); // segwit marker
        bytes_written++;
    }
    if(is_segwit && s->available() && bytes_written+offset == 5){
        s->write(0x01); // segwit flag
        bytes_written++;
    }
    size_t cur_offset = 4+2*is_segwit;
    size_t l = writeVarInt(inputsNumber, arr, 10);
    if(s->available() && bytes_written+offset < cur_offset+l){
        s->write(arr[bytes_written+offset-cur_offset]);
        bytes_written++;
    }
    cur_offset+=l;
    for(unsigned int i=0; i<inputsNumber; i++){
        l = txIns[i].length();
        if(s->available() && bytes_written+offset < cur_offset+l){
            bytes_written += s->serialize(&txIns[i], bytes_written+offset-cur_offset);
        }
        cur_offset+=l;
    }
    l = writeVarInt(outputsNumber, arr, 10);
    if(s->available() && bytes_written+offset < cur_offset+l){
        s->write(arr[bytes_written+offset-cur_offset]);
        bytes_written++;
    }
    cur_offset += l;
    for(unsigned int i=0; i<outputsNumber; i++){
        l = txOuts[i].length();
        if(s->available() && bytes_written+offset < cur_offset+l){
            bytes_written += s->serialize(&txOuts[i], bytes_written+offset-cur_offset);
        }
        cur_offset+=l;
    }
    if(is_segwit){
        for(unsigned int i=0; i<inputsNumber; i++){
            l = txIns[i].witness.length();
            if(s->available() && bytes_written+offset < cur_offset+l){
                bytes_written += s->serialize(&txIns[i].witness, bytes_written+offset-cur_offset);
            }
            cur_offset+=l;
        }
    }
    intToLittleEndian(locktime, arr, 4);
    while(s->available() && bytes_written+offset < cur_offset+4){
        s->write(arr[bytes_written+offset-cur_offset]);
        bytes_written++;
    }
    return bytes_written;
}
size_t Tx::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        clear();
        bytes_parsed = 0;
        version = 0;
        locktime = 0;
        segwit_flag = 0; // keep segwit flag during parsing...
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    while(s->available() && bytes_read+bytes_parsed<4){
        uint8_t c = s->read();
        version += (c << (8*(bytes_read+bytes_parsed)));
        bytes_read++;
    }
    if(s->available() && bytes_read+bytes_parsed == 4){
        uint8_t c = s->read();
        bytes_read++;
        if(c == 0x00){ // segwit!
            segwit_flag = 1;
        }else{
            inputsNumber = c; // FIXME: should be varint, but 255 inputs will kill the MCU...
            txIns = new TxIn[inputsNumber];
            for(unsigned int i=0; i<inputsNumber; i++){ // this will at least set all txins to PARSING_INCOMPLETE
                bytes_read += s->parse(&txIns[i]);
            }
        }
    }
    if(s->available() && segwit_flag > 0 && bytes_read+bytes_parsed == 5){
        uint8_t c = s->read();
        bytes_read++;
        if(c != 0x01){ // unsupported segwit version
            status = PARSING_FAILED;
            bytes_parsed+=bytes_read;
            return bytes_read;
        }
    }
    if(s->available() && segwit_flag > 0 && bytes_read+bytes_parsed == 6){
        inputsNumber = s->read();
        bytes_read++;
        txIns = new TxIn[inputsNumber];
        for(unsigned int i=0; i<inputsNumber; i++){ // this will at least set all txins to PARSING_INCOMPLETE
            bytes_read += s->parse(&txIns[i]);
        }
    }
    for(unsigned int i=0; i<inputsNumber; i++){
        if(s->available() && txIns[i].getStatus() == PARSING_INCOMPLETE){
            bytes_read += s->parse(&txIns[i]);
        }
        if(txIns[i].getStatus() == PARSING_FAILED){
            status = PARSING_FAILED;
            bytes_parsed+=bytes_read;
            return bytes_read;
        }
    }
    size_t current_offset = 5+2*segwit_flag;
    for(unsigned int i=0; i<inputsNumber; i++){
        current_offset += txIns[i].length();
    }
    if(s->available() && bytes_read+bytes_parsed == current_offset){
        outputsNumber = s->read();
        bytes_read++;
        txOuts = new TxOut[outputsNumber];
        for(unsigned int i=0; i<outputsNumber; i++){ // this will at least set all txouts to PARSING_INCOMPLETE
            bytes_read += s->parse(&txOuts[i]);
        }
    }
    for(unsigned int i=0; i<outputsNumber; i++){
        if(s->available() && txOuts[i].getStatus() == PARSING_INCOMPLETE){
            bytes_read += s->parse(&txOuts[i]);
        }
        if(txOuts[i].getStatus() == PARSING_FAILED){
            status = PARSING_FAILED;
            bytes_parsed+=bytes_read;
            return bytes_read;
        }
    }
    current_offset++;
    for(unsigned int i=0; i<outputsNumber; i++){
        current_offset += txOuts[i].length();
    }
    if(segwit_flag > 0 && bytes_read+bytes_parsed == current_offset){
        for(unsigned int i=0; i<inputsNumber; i++){ // this will at least set all txins witnesses to PARSING_INCOMPLETE
            bytes_read += s->parse(&txIns[i].witness);
        }
    }
    if(segwit_flag > 0){
        for(unsigned int i=0; i<inputsNumber; i++){
            if(s->available() && txIns[i].witness.getStatus() == PARSING_INCOMPLETE){
                bytes_read += s->parse(&txIns[i].witness);
            }
            if(txIns[i].witness.getStatus() == PARSING_FAILED){
                status = PARSING_FAILED;
                ubtc_errno = UBTC_ERR_TX_GLOBAL | UBTC_ERR_TX_SCRIPT;
                bytes_parsed+=bytes_read;
                return bytes_read;
            }
        }
        for(unsigned int i=0; i<inputsNumber; i++){
            current_offset += txIns[i].witness.length();
        }
    }
    while(s->available() && bytes_parsed+bytes_read < current_offset+4){
        uint8_t c = s->read();
        locktime += (c << (8*(bytes_read+bytes_parsed-current_offset)));
        bytes_read++;
    }
    current_offset+= 4;
    if(bytes_read+bytes_parsed == current_offset){
        bool completed = true;
        for(unsigned int i=0; i<inputsNumber; i++){
            if(txIns[i].getStatus() != PARSING_DONE){
                completed = false;
            }
            if(segwit_flag > 0){
                if(txIns[i].witness.getStatus() != PARSING_DONE){
                    completed = false;
                }
            }
        }
        for(unsigned int i=0; i<outputsNumber; i++){
            if(txOuts[i].getStatus() != PARSING_DONE){
                completed = false;
            }
        }
        if(completed){
            status = PARSING_DONE;
        }
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}
int Tx::sigHash(uint8_t h[32], uint8_t inputIndex, const Script scriptPubkey, SigHashType sighash) const{
    Script empty;
    DoubleSha s;
    s.begin();

    uint8_t arr[10];

    intToLittleEndian(version, arr, 4);
    s.write(arr, 4);
    size_t l = writeVarInt(inputsNumber, arr, 10);
    s.write(arr, l);
    for(size_t i=0; i<inputsNumber; i++){
        TxIn t = txIns[i];
        if(i == inputIndex){
            t.scriptSig = scriptPubkey;
        }else{
            t.scriptSig = empty;
        }
        s.serialize(&t, 0);
    }
    l = writeVarInt(outputsNumber, arr, 10);
    s.write(arr, l);
    for(size_t i=0; i<outputsNumber; i++){
        s.serialize(&txOuts[i], 0);
    }
    intToLittleEndian(locktime, arr, 4);
    s.write(arr, 4);
    intToLittleEndian(sighash, arr, 4);
    s.write(arr, 4);
    s.end(h);
    return 32;
}
int Tx::hash(uint8_t * h) const{
    DoubleSha s;
    s.begin();

    uint8_t arr[10];

    intToLittleEndian(version, arr, 4);
    s.write(arr, 4);
    size_t l = writeVarInt(inputsNumber, arr, 10);
    s.write(arr, l);
    for(unsigned int i=0; i<inputsNumber; i++){
        s.serialize(&txIns[i], 0);
    }
    l = writeVarInt(outputsNumber, arr, 10);
    s.write(arr, l);
    for(unsigned int i=0; i<outputsNumber; i++){
        s.serialize(&txOuts[i], 0);
    }
    intToLittleEndian(locktime, arr, 4);
    s.write(arr, 4);
    s.end(h);
    return 32;
}
int Tx::whash(uint8_t * h) const{
    DoubleSha s;
    s.begin();

    uint8_t arr[10];

    intToLittleEndian(version, arr, 4);
    s.write(arr, 4);
    arr[0] = 0; // marker
    arr[1] = 1; // flag
    s.write(arr, 2);
    size_t l = writeVarInt(inputsNumber, arr, 10);
    s.write(arr, l);
    for(unsigned int i=0; i<inputsNumber; i++){
        s.serialize(&txIns[i], 0);
    }
    l = writeVarInt(outputsNumber, arr, 10);
    s.write(arr, l);
    for(unsigned int i=0; i<outputsNumber; i++){
        s.serialize(&txOuts[i], 0);
    }
    for(unsigned int i=0; i<inputsNumber; i++){
        s.serialize(&txIns[i].witness, 0);
    }
    intToLittleEndian(locktime, arr, 4);
    s.write(arr, 4);
    s.end(h);
    return 32;
}
int Tx::txid(uint8_t * id_arr) const{
    uint8_t h[32];
    hash(h);
    for(uint8_t i=0;i<32;i++){
        id_arr[i] = h[31-i];
    }
    return 32;
}
int Tx::wtxid(uint8_t * id_arr) const{
    uint8_t h[32];
    whash(h);
    for(uint8_t i=0;i<32;i++){
        id_arr[i] = h[31-i];
    }
    return 32;
}

#if USE_ARDUINO_STRING || USE_STD_STRING
String Tx::txid() const{
    uint8_t id[32];
    txid(id);
    return toHex(id, 32);
}
String Tx::wtxid() const{
    uint8_t id[32];
    wtxid(id);
    return toHex(id, 32);
}
#endif

uint8_t Tx::addInput(const TxIn txIn){
    TxIn * arr = new TxIn[inputsNumber+1];
    for(unsigned int i=0; i<inputsNumber; i++){
        arr[i] = txIns[i];
    }
    arr[inputsNumber] = txIn;
    if(inputsNumber > 0){
        delete [] txIns;
    }
    txIns = arr;
    inputsNumber++;
    return inputsNumber;
}
uint8_t Tx::addOutput(const TxOut txOut){
    TxOut * arr = new TxOut[outputsNumber+1];
    for(unsigned int i=0; i<outputsNumber; i++){
        arr[i] = txOuts[i];
    }
    arr[outputsNumber] = txOut;
    if(outputsNumber > 0){
        delete [] txOuts;
    }
    txOuts = arr;
    outputsNumber++;
    return outputsNumber;
}

int Tx::hashPrevouts(uint8_t h[32]) const{
    DoubleSha s;
    s.begin();
    for(size_t i=0; i<inputsNumber; i++){
        s.write(txIns[i].hash, 32);
        uint8_t arr[4];
        intToLittleEndian(txIns[i].outputIndex, arr, 4);
        s.write(arr, 4);
    }
    s.end(h);
    return 32;
}

int Tx::hashSequence(uint8_t h[32]) const{
    DoubleSha s;
    s.begin();
    for(size_t i=0; i<inputsNumber; i++){
        uint8_t arr[4];
        intToLittleEndian(txIns[i].sequence, arr, 4);
        s.write(arr, 4);
    }
    s.end(h);
    return 32;
}

int Tx::hashOutputs(uint8_t h[32]) const{
    DoubleSha s;
    s.begin();
    for(size_t i=0; i<outputsNumber; i++){
        s.serialize(&txOuts[i], 0);
    }
    s.end(h);
    return 32;
}

int Tx::sigHashSegwit(uint8_t h[32], uint8_t inputIndex, const Script scriptPubKey, uint64_t amount, SigHashType sighash) const{
    DoubleSha s;
    s.begin();
    uint8_t arr[8];
    intToLittleEndian(version, arr, 4);
    s.write(arr, 4);

    hashPrevouts(h);
    s.write(h, 32);

    hashSequence(h);
    s.write(h, 32);

    s.write(txIns[inputIndex].hash, 32);
    intToLittleEndian(txIns[inputIndex].outputIndex, arr, 4);
    s.write(arr, 4);
    s.serialize(&scriptPubKey, 0);

    intToLittleEndian(amount, arr, 8);
    s.write(arr, 8);
    intToLittleEndian(txIns[inputIndex].sequence, arr, 4);
    s.write(arr, 4);

    hashOutputs(h);
    s.write(h, 32);

    intToLittleEndian(locktime, arr, 4);
    s.write(arr, 4);
    intToLittleEndian(sighash, arr, 4);
    s.write(arr, 4);

    s.end(h);
    return 32;
}

Signature Tx::signInput(uint8_t inputIndex, const PrivateKey pk, const Script redeemScript, SigHashType sighash){
    uint8_t h[32];
    sigHash(h, inputIndex, redeemScript, sighash);

    PublicKey pubkey = pk.publicKey();
    Signature sig = pk.sign(h);

    Script sc;
    sc.push(sig);
    sc.push(pubkey);
    // push script itself for timelocks and other single-key things if P2SH
    if(redeemScript.type() != P2PKH){
        sc.push(redeemScript);
    }

    txIns[inputIndex].scriptSig = sc;

    return sig;
}
Signature Tx::signSegwitInput(uint8_t inputIndex, const PrivateKey pk, const Script redeemScript, uint64_t amount, ScriptType type, SigHashType sighash){
    uint8_t h[32];

    ScriptType redeem_type = redeemScript.type();
    if(redeem_type == P2WPKH){
        Script sc(pk.publicKey(), P2PKH);
        sigHashSegwit(h, inputIndex, sc, amount, sighash);
    }else{
        sigHashSegwit(h, inputIndex, redeemScript, amount, sighash);
    }

    PublicKey pubkey = pk.publicKey();
    Signature sig = pk.sign(h);

    if((type == P2SH_P2WPKH) || (type == P2SH_P2WSH)){
        Script script_sig;
        if(redeem_type == P2WPKH){
            script_sig.push(redeemScript);
        }else{
            Script sc(redeemScript, P2WSH);
            script_sig.push(sc);
        }
        txIns[inputIndex].scriptSig = script_sig;
    }else{
        Script empty;
        txIns[inputIndex].scriptSig = empty;
    }

    Witness w;
    w.push(sig);
    w.push(pubkey);
    if(redeem_type != P2WPKH){
        w.push(redeemScript);
    }
    txIns[inputIndex].witness = w;

    return sig;
}
