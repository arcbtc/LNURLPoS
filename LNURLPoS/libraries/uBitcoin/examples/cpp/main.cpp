#include <iostream>
#include "Bitcoin.h"
#include "PSBT.h"

#include <stdint.h>
#include <stdlib.h>

// You can define your random function to improve side-channel resistance
extern "C" {

    // use system random function
    uint32_t random32(void){
        return (uint32_t)rand();
    }

}

using namespace std;

char mnemonic[] = "flight canvas heart purse potato mixed offer tooth maple blue kitten salute almost staff physical remain coral clump midnight rotate innocent shield inch ski";

int main() {
    // convert mnemonic to xprv
    cout << "Your mnemonic: " << endl << mnemonic << endl;
    HDPrivateKey hd(mnemonic, "");
    cout << "Your xprv: " << endl << string(hd) << endl;
    // derive account xpub
    char derivation[] = "m/84h/0h/0h";
    HDPublicKey xpub = hd.derive(derivation).xpub();
    cout << "Account xpub at path " << derivation << ":" << endl;
    cout << string(xpub) << endl;
    // print first 5 receiving addresses
    HDPublicKey recv_xpub = xpub.child(0);
    for (uint32_t i = 0; i < 5; i++)
    {
        cout << "Address #" << i << ": " << recv_xpub.child(i).address() << endl;
    }

    // signing PSBT transaction
    PSBT psbt;
    psbt.parseBase64(string("cHNidP8BAHICAAAAAUswu6MJzSuKPVEDD3cxwoPYynvQOnUP1xIga/Qyv+icAAAAAAD9////AqCGAQAAAAAAF6kUxD/8BOj9UucJiNpagTRzluc4gvKHErsNAAAAAAAWABTZJKDg5Ayidmusul21PNw16zUy2PDlFwAAAQEfQEIPAAAAAAAWABRw5Uq2fGtsTutqAZqDMypDuQmxmSIGA3s6OgE8GCKOcHDJe7XY0q/i/XSe6e933ErCDCCKR5WoGARkI4xUAACAAQAAgAAAAIAAAAAAAAAAAAAAIgID07CelU8+BYAL87tK7Ec0+NfjojPZC/11wjTfcCoTK/4YBGQjjFQAAIABAACAAAAAgAEAAAAAAAAAAA=="));
    // check parsing is ok
    if(!psbt){
        cout << "Failed parsing transaction" << endl;
        exit(EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    cout << "Transactions details:" << endl;
    // going through all outputs
    cout << "Outputs:" << endl;
    for(unsigned int i = 0; i < psbt.tx.outputsNumber; i++){
        // print addresses
        cout << psbt.tx.txOuts[i].address(&Testnet);
        if(psbt.txOutsMeta[i].derivationsLen > 0){ // there is derivation path
            // considering only single key for simplicity
            PSBTDerivation der = psbt.txOutsMeta[i].derivations[0];
            HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
            if(pub.address() == psbt.tx.txOuts[i].address()){
                cout << " (change) ";
            }
        }
        cout << " -> " << psbt.tx.txOuts[i].btcAmount()*1e3 << " mBTC" << endl;
    }
    cout << "Fee: " << psbt.fee() << " sat" << endl;

    psbt.sign(hd);
    cout << "Signed transaction:" << endl << psbt.toBase64() << endl; // now you can combine and finalize PSBTs in Bitcoin Core

    return 0;
}
