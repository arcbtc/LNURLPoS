/*
 * This example shows how to derive master private key from the recovery seed
 * Generate a random recovery seed i.e. on https://iancoleman.io/bip39/
 * and check the master private key, account private key, account public key
 * and first address.
 */
#include "Bitcoin.h"

void printHD(String mnemonic, String password = ""){

  HDPrivateKey hd(mnemonic, password);

  if(!hd){ // check if it is valid
    Serial.println("Invalid xpub");
    return;
  }
  Serial.println("Mnemonic:");
  Serial.println(mnemonic);
  Serial.print("Password: \"");
  Serial.println(password+"\"");
  Serial.println("Root private key:");
  Serial.println(hd);

  Serial.println("bip84 master private key:");
  HDPrivateKey account = hd.derive("m/84'/0'/0'/");
  Serial.println(account);
  
  Serial.println("bip84 master public key:");
  Serial.println(account.xpub());
  
  Serial.println("first address:");
  Serial.println(account.derive("m/0/0/").address());
  
  Serial.println("\n");
}

void setup() {
  Serial.begin(115200);
  printHD("arch volcano urge cradle turn labor skin secret squeeze denial jacket vintage fix glad lemon", "my secret password");

  // entropy bytes to mnemonic
  uint8_t arr[] = {'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};
  String mn = mnemonicFromEntropy(arr, sizeof(arr));
  Serial.println(mn);
  uint8_t out[16];
  size_t len = mnemonicToEntropy(mn, out, sizeof(out));
  Serial.println(toHex(out, len));

}

void loop() {
  delay(100);
}
