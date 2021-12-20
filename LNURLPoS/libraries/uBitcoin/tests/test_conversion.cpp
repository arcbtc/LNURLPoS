#include "minunit.h"
#include "Conversion.h" // to print byte arrays in hex format

using std::string;

const char message[] = "Hello world!"; // message to convert
const uint8_t * bytes = (const uint8_t *)message;
const char b58[] = "2NEpo7TZRhna7vSvL";
const char b58check[] = "9wWTEnNTWna86WmtFaRbXa";
const char b64[] = "SGVsbG8gd29ybGQh";

uint8_t out[100];
size_t sz;
string s;

MU_TEST(test_base58) {
  s = toBase58(bytes, strlen(message));
  mu_assert(strcmp(s.c_str(), b58) == 0, "toBase58 conversion is invalid");

  sz = fromBase58(b58, out, sizeof(out));
  mu_assert(sz == strlen(message), "fromBase58 size is wrong");
  mu_assert(memcmp(out, bytes, sz) == 0, "fromBase58 decoding is wrong");

  s = toBase58Check(bytes, strlen(message));
  mu_assert(strcmp(s.c_str(), b58check) == 0, "toBase58 conversion is invalid");

  sz = fromBase58Check(b58check, out, sizeof(out));
  mu_assert(sz == strlen(message), "fromBase58Check size is wrong");
  mu_assert(memcmp(out, bytes, sz) == 0, "fromBase58Check decoding is wrong");

  sz = fromBase58(b58check, out, sizeof(out));
  mu_assert(sz == (strlen(message)+4), "fromBase58 size is wrong");
  mu_assert(memcmp(out, bytes, sz-4) == 0, "fromBase58 decoding is wrong");
}

MU_TEST(test_base64) {
  s = toBase64(bytes, strlen(message));
  mu_assert(strcmp(s.c_str(), b64) == 0, "toBase64 conversion is invalid");

  sz = fromBase64(b64, out, sizeof(out));
  mu_assert(sz == strlen(message), "fromBase64 size is wrong");
  mu_assert(memcmp(out, bytes, sz) == 0, "fromBase64 decoding is wrong");

  char *msgs[] = {
    "Man", "Ma", "M", "light w", "light wo", "light wor", "light work", "light work.",
    "\x6c\x68\x40\xa2\xc4\xbe\xfe\xa7\x85\x30\x20", NULL
  };
  char *b64msgs[] = {
    "TWFu", "TWE=", "TQ==", "bGlnaHQgdw==", "bGlnaHQgd28=", "bGlnaHQgd29y", "bGlnaHQgd29yaw==", "bGlnaHQgd29yay4=",
    "bGhAosS+/qeFMCA=", NULL
  };
  char *b64msgs_url[] = {
    "TWFu", "TWE", "TQ", "bGlnaHQgdw", "bGlnaHQgd28", "bGlnaHQgd29y", "bGlnaHQgd29yaw", "bGlnaHQgd29yay4",
    "bGhAosS-_qeFMCA", NULL
  };
  size_t i = 0;
  while(msgs[i] != NULL){
    char *msg = msgs[i];
    s = toBase64((uint8_t*)msg, strlen(msg));
    mu_assert(strcmp(s.c_str(), b64msgs[i]) == 0, "toBase64 conversion is invalid");

    sz = fromBase64(b64msgs[i], out, sizeof(out));
    mu_assert(sz == strlen(msg), "fromBase64 size is wrong");
    mu_assert(memcmp(out, msg, sz) == 0, "fromBase64 decoding is wrong");

    s = toBase64((uint8_t*)msg, strlen(msg), BASE64_NOPADDING | BASE64_URLSAFE);
    size_t l = toBase64Length((uint8_t*)msg, strlen(msg), BASE64_NOPADDING | BASE64_URLSAFE);
    mu_assert(strcmp(s.c_str(), b64msgs_url[i]) == 0, "toBase64 urlsafe conversion is invalid");

    sz = fromBase64(b64msgs_url[i], out, sizeof(out), BASE64_NOPADDING | BASE64_URLSAFE);
    mu_assert(sz == strlen(msg), "fromBase64 size is wrong");
    mu_assert(memcmp(out, msg, sz) == 0, "fromBase64 urlsafe decoding is wrong");

    i++;
  }
}

MU_TEST(test_nullptr) {
  s = toBase64(NULL, strlen(message));
  mu_assert(strcmp(s.c_str(), "") == 0, "toBase64 doesn't handle nullptr properly");

  sz = fromBase64(b64, NULL, sizeof(out));
  mu_assert(sz == 0, "fromBase64 doesn't handle nullptr properly");

  s = toBase58(NULL, strlen(message));
  mu_assert(strcmp(s.c_str(), "") == 0, "toBase58 doesn't handle nullptr properly");

  sz = fromBase58(b58, NULL, sizeof(out));
  mu_assert(sz == 0, "fromBase58 doesn't handle nullptr properly");

  s = toBase58Check(NULL, strlen(message));
  mu_assert(strcmp(s.c_str(), "") == 0, "toBase58Check doesn't handle nullptr properly");

  sz = fromBase58Check(b58check, NULL, sizeof(out));
  mu_assert(sz == 0, "fromBase58Check doesn't handle nullptr properly");
}

MU_TEST_SUITE(test_conversion) {
  MU_RUN_TEST(test_base58);
  MU_RUN_TEST(test_base64);
  MU_RUN_TEST(test_nullptr);
}

int main(int argc, char *argv[]) {
  MU_RUN_SUITE(test_conversion);
  MU_REPORT();
  return MU_EXIT_CODE;
}
