#include "minunit.h"
#include "Hash.h"  // all single-line hashing algorithms
#include "Conversion.h" // to print byte arrays in hex format

using namespace std;

char message[] = "Hello world!"; // message to hash

MU_TEST(test_sha256) {
  uint8_t hash[32];
  int hashLen = sha256(message, hash);
  string hexresult = toHex(hash, hashLen);
  char expected[] = "c0535e4be2b79ffd93291305436bf889314e4a3faec05ecffcbb7df31ad9e51a";
  mu_assert(strcmp(expected, hexresult.c_str()) == 0, "sha256 with string is invalid");

  // using bytes
  uint8_t hash2[32];
  uint8_t *bytes = (uint8_t *)message;
  size_t byteslen = strlen(message);
  
  sha256(bytes, byteslen, hash2);
  mu_assert(memcmp(hash, hash2, sizeof(hash)) == 0, "sha256 with bytes is invalid");

  // sending data in pieces
  SHA256 h;
  h.begin();
  h.write(bytes, 5); // add first 5 bytes to hash
  h.write(bytes+5, byteslen-5); // add the rest
  h.end(hash2); // result will be stored in the hash array
  mu_assert(memcmp(hash, hash2, sizeof(hash)) == 0, "sha256 in pieces is invalid");
}

MU_TEST(test_ripemd160) {
  uint8_t hash[20];
  int hashLen = rmd160(message, hash);
  string hexresult = toHex(hash, hashLen);
  mu_assert(strcmp(hexresult.c_str(), "7f772647d88750add82d8e1a7a3e5c0902a346a3") == 0, "ripemd160 is wrong");
}

MU_TEST(test_hash160) {
  uint8_t hash[20];
  int hashLen = hash160(message, hash);
  string hexresult = toHex(hash, hashLen);
  mu_assert(strcmp(hexresult.c_str(), "621281c15fb62d5c6013ea29007491e8b174e1b9") == 0, "hash160 is wrong");
}

MU_TEST(test_doublesha256) {
  uint8_t hash[32];
  int hashLen = doubleSha(message, hash);
  string hexresult = toHex(hash, hashLen);
  mu_assert(strcmp(hexresult.c_str(), "7982970534e089b839957b7e174725ce1878731ed6d700766e59cb16f1c25e27") == 0, "double sha256 is wrong");
}

MU_TEST(test_sha512) {
  uint8_t hash[64];
  int hashLen = sha512(message, hash);
  string hexresult = toHex(hash, hashLen);
  mu_assert(strcmp(hexresult.c_str(), "f6cde2a0f819314cdde55fc227d8d7dae3d28cc556222a0a8ad66d91ccad4aad6094f517a2182360c9aacf6a3dc323162cb6fd8cdffedb0fe038f55e85ffb5b6") == 0, "sha512 is wrong");
}

MU_TEST_SUITE(test_hash) {
  MU_RUN_TEST(test_sha256);
  MU_RUN_TEST(test_ripemd160);
  MU_RUN_TEST(test_hash160);
  MU_RUN_TEST(test_doublesha256);
  MU_RUN_TEST(test_sha512);
}

int main(int argc, char *argv[]) {
  MU_RUN_SUITE(test_hash);
  MU_REPORT();
  return MU_EXIT_CODE;
}
