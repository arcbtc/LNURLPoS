#include <Hash.h>  // all single-line hashing algorithms
#include <Conversion.h> // to print byte arrays in hex format

void setup() {
  Serial.begin(9600);
  while(!Serial){
    ; // wait for serial port to open
  }
  String message = "Hello world!"; // message to hash
  byte hash[64] = { 0 }; // hash
  int hashLen = 0;

  // sha-256
  hashLen = sha256(message, hash);
  Serial.println("SHA-256:   " + toHex(hash, hashLen));
  Serial.println("Should be: c0535e4be2b79ffd93291305436bf889314e4a3faec05ecffcbb7df31ad9e51a");

  // you can also pass byte arrays to the hash function
  // and update piece by piece
  byte msg2[] = {1, 2, 3, 4, 5, 0xFA, 0xB0, 0x0B, 0x51};
  // hash in one line
  sha256(msg2, sizeof(msg2), hash);
  // you can also create a class instance and use .write method
  SHA256 h;
  h.begin();
  h.write(msg2, 5); // add first 5 bytes to hash
  h.write(msg2+5, 4); // add another 4 bytes
  h.end(hash); // result will be stored in the hash array

  // ripemd-160
  hashLen = rmd160(message, hash);
  Serial.println("RMD-160:   " + toHex(hash, hashLen));
  Serial.println("Should be: 7f772647d88750add82d8e1a7a3e5c0902a346a3");

  // hash160(msg) = rmd160(sha256(message))
  hashLen = hash160(message, hash);
  Serial.println("Hash160:   " + toHex(hash, hashLen));
  Serial.println("Should be: 621281c15fb62d5c6013ea29007491e8b174e1b9");

  // sha256(sha256(message))
  hashLen = doubleSha(message, hash);
  Serial.println("DoubleSha: " + toHex(hash, hashLen));
  Serial.println("Should be: 7982970534e089b839957b7e174725ce1878731ed6d700766e59cb16f1c25e27");

  // sha512
  hashLen = sha512(message, hash);
  Serial.println("Sha512:    " + toHex(hash, hashLen));
  Serial.println("Should be: f6cde2a0f819314cdde55fc227d8d7dae3d28cc556222a0a8ad66d91ccad4aad6094f517a2182360c9aacf6a3dc323162cb6fd8cdffedb0fe038f55e85ffb5b6");

  // sha512-hmac
  // here we use more c-style approach
  char key[] = "Bitcoin seed";
  hashLen = sha512Hmac((byte*)key, strlen(key), (byte*)message.c_str(), message.length(), hash);
  Serial.println("Sha512-HMAC: " + toHex(hash, hashLen));
  Serial.println("Should be:   f7fc496a2c17bd09a6328124dc6edebed987e7e93903deee0633a756f1ee81da0753334f6cfe226b5c712d893a68c547d3a5497cd73e1d010670c1e0e9d93a8a");
}

void loop() {

}
