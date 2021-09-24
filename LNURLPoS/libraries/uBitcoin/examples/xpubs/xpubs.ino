/*
 * This example shows how to derive bitcoin addresses from the master public key (bip32)
 * Enter account master public key to the serial console and get the addresses
 * Use this tool to check: https://iancoleman.io/bip39/
 */
#include "Bitcoin.h"

void printAddresses(String pub){

  HDPublicKey hd(pub);

  if(!hd){ // check if it is valid
    Serial.println("Invalid xpub");
    return;
  }

  Serial.println("Master public key:");
  Serial.println(hd);
  
  Serial.println("First 5 receiving addresses:");
  for(int i=0; i<5; i++){
    String path = String("m/0/")+i;
    Serial.print("Path:");
    Serial.println(path);
    Serial.print("Address: ");
    Serial.println(hd.derive(path).address());
    // Serial.print("Legacy: ");
    // Serial.println(hd.derive(path).legacyAddress());
    // Serial.print("Nested segwit: ");
    // Serial.println(hd.derive(path).nestedSegwitAddress());
    // Serial.print("Native segwit: ");
    // Serial.println(hd.derive(path).segwitAddress());
  }
  
  Serial.println("\n");
}

void setup() {
  Serial.begin(115200);
  // bip 44
  printAddresses("xpub6BoiLSuHTDytgQejDauyXkBqQ4xTw2tSFKgkHfmZky7jDQQecUKwbFocxZXSJMCSp8gFNBbD9Squ3etZbJkE2qQqVBrypLjWJaWUNmHh3LT");
  // bip 49
  printAddresses("ypub6XMsccDqTfZCUz5m4VjbRLebnjFTLDpeKTgRJuUtAxkpQicB7p4ZwHdmimRuMTcunPfdpzgpVt7DCDKRRffsgmWavWLXccumbyYazC3wh5N");
  // bip 84
  printAddresses("zpub6rcGDMmj82CUJT1uV2mCcsN4EPTgBnJjciDWpYv12yCAPi9TEG5KLPF5iPtqzL6hNmaa5ZGfhJCHctoex7cGgqVxsyWcCDUNUDhjaYRQXzV");
  Serial.println("Enter your master public key:");
}

void loop() {
  if(Serial.available()){
    String pub = Serial.readStringUntil('\n');
    Serial.println(pub);
    if(pub.length() > 0){
      printAddresses(pub);
    }
  }
  delay(100);
}
