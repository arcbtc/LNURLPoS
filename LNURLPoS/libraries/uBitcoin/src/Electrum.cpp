#include "Electrum.h"

ElectrumTx::ElectrumTx(ElectrumTx const &other){
    tx = other.tx;
    is_segwit = false;
    txInsMeta = new ElectrumInputMetadata[tx.inputsNumber];
    for(unsigned int i=0; i<tx.inputsNumber; i++){
        txInsMeta[i] = other.txInsMeta[i];
    }
    status = other.status;
    bytes_parsed = other.bytes_parsed;
}
ElectrumTx& ElectrumTx::operator=(ElectrumTx const &other){
    if (this == &other){ return *this; } // self-assignment
    if(tx.inputsNumber > 0){
        delete [] txInsMeta;
    }
    tx = other.tx;
    txInsMeta = new ElectrumInputMetadata[tx.inputsNumber];
    for(unsigned int i=0; i<tx.inputsNumber; i++){
        txInsMeta[i] = other.txInsMeta[i];
    }
    status = other.status;
    bytes_parsed = other.bytes_parsed;
    return *this;
}

size_t ElectrumTx::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        if(tx.inputsNumber > 0){
            delete [] txInsMeta;
        }
        txInsMeta = NULL;
        tx = Tx();
        bytes_parsed = 0;
        is_segwit = false;
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    uint8_t prefix[] = {0x45, 0x50, 0x54, 0x46, 0xFF, 0x00};
    while(s->available() && bytes_read+bytes_parsed<6){
        uint8_t c = s->read();
        bytes_read++;
        if(c != prefix[bytes_read+bytes_parsed-1]){
            status = PARSING_FAILED;
            bytes_parsed += bytes_read;
            return bytes_read;
        }
    }
    if(!is_segwit && s->available() && status == PARSING_INCOMPLETE && tx.getStatus() != PARSING_FAILED){
        bytes_read += s->parse(&tx);
        if(tx.getStatus() == PARSING_DONE){
            status = PARSING_DONE;
            is_segwit = false;
            txInsMeta = new ElectrumInputMetadata[tx.inputsNumber];
            for(unsigned i=0; i<tx.inputsNumber; i++){
                if(tx.txIns[i].scriptSig.length() != 88){ // no idea how to parse other things
                    status = PARSING_FAILED;
                    bytes_parsed+=bytes_read;
                    return bytes_read;
                }else{
                    uint8_t arr[88];
                    tx.txIns[i].scriptSig.serialize(arr, sizeof(arr));
                    txInsMeta[i].hd.parse(arr+6,sizeof(arr)-6);
                    if(txInsMeta[i].hd.getStatus() != PARSING_DONE){
                        status = PARSING_FAILED;
                        bytes_parsed+=bytes_read;
                        return bytes_read;
                    }
                    txInsMeta[i].derivation[0] = arr[84] + (arr[85] << 8);
                    txInsMeta[i].derivation[1] = arr[86] + (arr[87] << 8);
                    tx.txIns[i].scriptSig = Script();
                }
            }
        }
    }
    if(tx.getStatus() == PARSING_FAILED){
        if(tx.inputsNumber > 0 && tx.txIns[0].witness.getStatus() == PARSING_FAILED){
            for(unsigned int i=0; i<tx.inputsNumber; i++){
                tx.txIns[i].witness = Witness();
            }
            tx.setStatus(PARSING_DONE);
            tx.locktime = 0;
            is_segwit = true;
            txInsMeta = new ElectrumInputMetadata[tx.inputsNumber];
            for(unsigned int i=0; i<tx.inputsNumber; i++){
                txInsMeta[i].amount = 0;
                txInsMeta[i].derivation[0] = 0;
                txInsMeta[i].derivation[1] = 0;
            }
        }else{
            status = PARSING_FAILED;
            bytes_parsed += bytes_read;
            return bytes_read;
        }
    }
    if(is_segwit && tx.getStatus() == PARSING_DONE){
        size_t start = 6 + tx.length() + 2 - 4; // locktime is not parsed yet
        for(unsigned int i = 0; i < tx.inputsNumber; i++){
            while(s->available() && bytes_parsed+bytes_read < start + 5){
                s->read(); 
                bytes_read++;
            }
            start += 5;
            while(s->available() && bytes_parsed+bytes_read < start + 8){
                uint8_t c = s->read();
                txInsMeta[i].amount += (((uint64_t)c) << (8*(bytes_read+bytes_parsed-start)));
                bytes_read++;
            }
            start += 8;
            while(s->available() && bytes_parsed+bytes_read < start + 7){
                s->read(); 
                bytes_read++;
            }
            start += 7;
            while(s->available() && bytes_parsed+bytes_read < start + 78){
                bytes_read+=s->parse(&txInsMeta[i].hd);
            }
            start += 78;
            while(s->available() && bytes_parsed+bytes_read < start + 4){
                uint8_t c = s->read();
                size_t cur = bytes_parsed+bytes_read-start;
                txInsMeta[i].derivation[cur/2] += (c << (8 * (cur % 2)));
                bytes_read++;
            }
            start += 4;
        }
        while(s->available() && bytes_parsed+bytes_read < start + 4){
            uint8_t c = s->read();
            tx.locktime += (c << (8*(bytes_read+bytes_parsed-start)));
            bytes_read++;
        }
        start+=4;
        if(bytes_parsed+bytes_read == start){
            status = PARSING_DONE;
        }
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}
ElectrumTx::~ElectrumTx(){
    delete [] txInsMeta;
}
uint8_t ElectrumTx::sign(const HDPrivateKey account){
    uint8_t res = 0; // number of signed inputs
    for(unsigned int i=0; i<tx.inputsNumber; i++){
        HDPublicKey pub = account.xpub();
        ScriptType type = txInsMeta[i].hd.type;
        pub.type = type;
        if(pub == txInsMeta[i].hd){
            PrivateKey pk = account.child(txInsMeta[i].derivation[0]).child(txInsMeta[i].derivation[1]);
            if(type == P2PKH || type == P2SH || type == UNKNOWN_TYPE || type == DIRECT_SCRIPT){
                tx.signInput(i, pk);
            }else{
                tx.signSegwitInput(i, pk, txInsMeta[i].amount, type);
            }
            res++;
        }
    }
    return res;
}

uint64_t ElectrumTx::fee() const{
    uint64_t inputs_amount = 0;
    for(unsigned int i=0; i < tx.inputsNumber; i++){
        if(txInsMeta[i].amount == 0){
            return 0;
        }
        inputs_amount += txInsMeta[i].amount;
    }
    uint64_t outputs_amount = 0;
    for(unsigned int i=0; i < tx.outputsNumber; i++){
        outputs_amount += tx.txOuts[i].amount;
    }
    if(inputs_amount < outputs_amount){
        return 0;
    }
    return inputs_amount-outputs_amount;
}

