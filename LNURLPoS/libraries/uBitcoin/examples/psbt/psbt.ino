/*
 * This example shows how parse and sign PSBT transaction
 */

#include "Bitcoin.h"
#include "PSBT.h"

HDPrivateKey hd("flight canvas heart purse potato mixed offer tooth maple blue kitten salute almost staff physical remain coral clump midnight rotate innocent shield inch ski", "");
PSBT psbt;

void setup() {
  Serial.begin(115200);
  psbt.parseBase64("cHNidP8BAHICAAAAAUswu6MJzSuKPVEDD3cxwoPYynvQOnUP1xIga/Qyv+icAAAAAAD9////AqCGAQAAAAAAF6kUxD/8BOj9UucJiNpagTRzluc4gvKHErsNAAAAAAAWABTZJKDg5Ayidmusul21PNw16zUy2PDlFwAAAQEfQEIPAAAAAAAWABRw5Uq2fGtsTutqAZqDMypDuQmxmSIGA3s6OgE8GCKOcHDJe7XY0q/i/XSe6e933ErCDCCKR5WoGARkI4xUAACAAQAAgAAAAIAAAAAAAAAAAAAAIgID07CelU8+BYAL87tK7Ec0+NfjojPZC/11wjTfcCoTK/4YBGQjjFQAAIABAACAAAAAgAEAAAAAAAAAAA==");
  // check parsing is ok
  if(!psbt){
    Serial.println("Failed parsing transaction");
    return;
  }
  Serial.println("Transactions details:");
  // going through all outputs
  Serial.println("Outputs:");
  for(int i=0; i<psbt.tx.outputsNumber; i++){
    // print addresses
    Serial.print(psbt.tx.txOuts[i].address(&Testnet));
    if(psbt.txOutsMeta[i].derivationsLen > 0){ // there is derivation path
      // considering only single key for simplicity
      PSBTDerivation der = psbt.txOutsMeta[i].derivations[0];
      HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
      if(pub.address() == psbt.tx.txOuts[i].address()){
        Serial.print(" (change) ");
      }
    }
    Serial.print(" -> ");
    Serial.print(psbt.tx.txOuts[i].btcAmount()*1e3);
    Serial.println(" mBTC");
  }
  Serial.print("Fee: ");
  Serial.print(float(psbt.fee())/100); // Arduino can't print 64-bit ints
  Serial.println(" bits");
  
  psbt.sign(hd);
  Serial.println(psbt.toBase64()); // now you can combine and finalize PSBTs in Bitcoin Core
}

void loop() {
}
